/**
 * @file TcpServer.cpp
 * @brief High-level TCP server implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpServer.h>
#include <array>
#include <cstdio>

namespace sockpp {

TcpServer::TcpServer() = default;

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::onConnection(ConnectionCallback callback) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    m_onConnection = std::move(callback);
}

void TcpServer::onMessage(MessageCallback callback) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void TcpServer::onDisconnection(DisconnectionCallback callback) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    m_onDisconnection = std::move(callback);
}

bool TcpServer::start(unsigned short port, IpAddress address) {
    if (m_running) return false;

    if (m_listener.listen(port, address) != Socket::Status::Done) {
        return false;
    }

    m_selector.add(m_listener);
    m_running = true;
    m_thread = std::thread(&TcpServer::serverLoop, this);
    return true;
}

void TcpServer::stop() {
    if (!m_running) return;

    m_running = false;

    // Wake up the selector so serverLoop can see m_running == false and exit.
    m_selector.clear();
    m_listener.close();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    // Collect remaining client IDs and clear the map.
    std::vector<ClientId> remainingIds;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        for (const auto& [id, _] : m_clients) {
            remainingIds.push_back(id);
        }
        m_clients.clear();
    }

    // Fire disconnection callbacks outside the lock.
    if (m_onDisconnection) {
        for (auto id : remainingIds) {
            try { m_onDisconnection(id); } catch (...) {}
        }
    }
}

bool TcpServer::isRunning() const {
    return m_running;
}

bool TcpServer::send(ClientId clientId, const void* data, std::size_t size) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end() || it->second.markedForClose) {
        return false;
    }
    return it->second.socket.send(data, size) == Socket::Status::Done;
}

void TcpServer::broadcast(const void* data, std::size_t size) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (auto& [id, client] : m_clients) {
        if (!client.markedForClose) {
            if (client.socket.send(data, size) != Socket::Status::Done) {
                client.markedForClose = true;
            }
        }
    }
}

void TcpServer::disconnect(ClientId clientId) {
    bool found = false;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end() && !it->second.markedForClose) {
            it->second.markedForClose = true;
            found = true;
        }
    }

    // Fire the callback outside the lock to prevent deadlock if the user calls
    // back into TcpServer from the disconnection handler.
    if (found && m_onDisconnection) {
        try { m_onDisconnection(clientId); } catch (...) {}
    }
}

std::size_t TcpServer::clientCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::size_t count = 0;
    for (const auto& [id, client] : m_clients) {
        if (!client.markedForClose) {
            ++count;
        }
    }
    return count;
}

void TcpServer::serverLoop() {
    std::array<char, 4096> buffer{};
    constexpr auto kTimeout = std::chrono::milliseconds(100);

    while (m_running) {
        try {
            // --- Phase 1: drain clients flagged for close by disconnect().
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                for (auto it = m_clients.begin(); it != m_clients.end();) {
                    if (it->second.markedForClose) {
                        m_selector.remove(it->second.socket);
                        auto cb = m_onDisconnection;
                        auto id = it->first;
                        it = m_clients.erase(it);

                        lock.unlock();
                        if (cb) {
                            try { cb(id); } catch (const std::exception& e) {
                                std::fprintf(stderr, "sockpp: disconnection callback exception: %s\n", e.what());
                            } catch (...) {
                                std::fprintf(stderr, "sockpp: disconnection callback unknown exception\n");
                            }
                        }
                        lock.lock();
                    } else {
                        ++it;
                    }
                }
            }

            // --- Phase 2: wait for activity.
            if (!m_selector.wait(kTimeout)) {
                continue;
            }

            // --- Phase 3: accept new connections.
            if (m_selector.isReady(m_listener)) {
                TcpSocket clientSocket;
                if (m_listener.accept(clientSocket) == Socket::Status::Done) {
                    const ClientId newId = m_nextClientId++;
                    const auto addr = clientSocket.getRemoteAddress().value_or(IpAddress::Any);

                    {
                        std::unique_lock<std::shared_mutex> lock(m_mutex);
                        ClientInfo info;
                        info.socket = std::move(clientSocket);
                        info.address = addr;
                        m_selector.add(info.socket);
                        m_clients[newId] = std::move(info);
                    }

                    // Capture addr before clientSocket was moved; use addr here.
                    if (m_onConnection) {
                        try { m_onConnection(newId, addr); } catch (...) {}
                    }
                }
            }

            // --- Phase 4: receive data from existing clients.
            // Store (clientId, receivedBytes) pairs; data is read from the stack buffer.
            struct PendingMessage {
                ClientId id;
                std::size_t size;
            };
            std::vector<PendingMessage> messagesToProcess;
            std::vector<ClientId> clientsToDisconnect;

            {
                std::shared_lock<std::shared_mutex> lock(m_mutex);
                for (auto& [id, client] : m_clients) {
                    if (client.markedForClose) continue;

                    if (m_selector.isReady(client.socket)) {
                        std::size_t received = 0;
                        const auto status = client.socket.receive(buffer.data(), buffer.size(), received);

                        if (status == Socket::Status::Done) {
                            if (received > 0 && m_onMessage) {
                                messagesToProcess.push_back({id, received});
                            }
                        } else {
                            clientsToDisconnect.push_back(id);
                        }
                    }
                }
            }

            // --- Phase 5: invoke message callbacks outside the lock.
            for (const auto& msg : messagesToProcess) {
                if (m_onMessage) {
                    try { m_onMessage(msg.id, buffer.data(), msg.size); } catch (...) {}
                }
            }

            // --- Phase 6: disconnect clients that dropped the connection.
            for (auto id : clientsToDisconnect) {
                disconnect(id);
            }

        } catch (const std::exception& ex) {
            std::fprintf(stderr, "sockpp: server loop exception: %s\n", ex.what());
        } catch (...) {
            std::fprintf(stderr, "sockpp: server loop unknown exception\n");
        }
    }
}

} // namespace sockpp
