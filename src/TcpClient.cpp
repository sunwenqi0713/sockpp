/**
 * @file TcpClient.cpp
 * @brief High-level TCP client implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpClient.h>
#include <sockpp/SocketSelector.h>
#include <array>
#include <cstdio>

namespace sockpp {

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
    disconnect();
}

void TcpClient::onConnected(ConnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onConnected = std::move(callback);
}

void TcpClient::onMessage(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void TcpClient::onDisconnected(DisconnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onDisconnected = std::move(callback);
}

void TcpClient::onError(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

bool TcpClient::connect(const std::string& host, unsigned short port, std::chrono::milliseconds timeout) {
    auto address = IpAddress::resolve(host);
    if (!address) {
        invokeError("Failed to resolve host: " + host);
        return false;
    }
    return connect(*address, port, timeout);
}

bool TcpClient::connect(IpAddress address, unsigned short port, std::chrono::milliseconds timeout) {
    disconnect();

    m_serverAddress = address;
    m_serverPort = port;
    m_timeout = timeout;
    m_stopRequested = false;

    const auto status = m_socket.connect(address, port, timeout);
    if (status != Socket::Status::Done) {
        invokeError("Failed to connect to server");
        return false;
    }

    m_connected = true;
    m_running = true;

    m_receiveThread = std::thread(&TcpClient::receiveLoop, this);

    if (m_onConnected) {
        try { m_onConnected(); } catch (...) {}
    }

    return true;
}

void TcpClient::disconnect() {
    m_stopRequested = true;
    m_running = false;
    m_connected = false;

    // Force-close the socket to unblock any pending recv() in the receive thread.
    m_socket.disconnect();

    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
}

bool TcpClient::isConnected() const {
    return m_connected;
}

bool TcpClient::send(std::string_view data) {
    return send(data.data(), data.size());
}

bool TcpClient::send(const void* data, std::size_t size) {
    if (!m_connected) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_socket.send(data, size) == Socket::Status::Done;
}

unsigned short TcpClient::getLocalPort() const {
    return m_socket.getLocalPort();
}

std::optional<IpAddress> TcpClient::getRemoteAddress() const {
    return m_socket.getRemoteAddress();
}

unsigned short TcpClient::getRemotePort() const {
    return m_socket.getRemotePort();
}

void TcpClient::setAutoReconnect(bool enable, std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_autoReconnect = enable;
    m_reconnectInterval = interval;
}

void TcpClient::invokeError(const std::string& msg) {
    if (m_onError) {
        try { m_onError(msg); } catch (...) {}
    }
}

void TcpClient::receiveLoop() {
    std::array<char, 4096> buffer{};
    SocketSelector selector;
    selector.add(m_socket);

    while (m_running && !m_stopRequested) {
        // Short timeout allows prompt exit when m_running becomes false.
        if (!selector.wait(std::chrono::milliseconds(100))) {
            continue;
        }

        if (!selector.isReady(m_socket)) {
            continue;
        }

        std::size_t received = 0;
        const Socket::Status status = m_socket.receive(buffer.data(), buffer.size(), received);

        if (status == Socket::Status::Done) {
            if (m_onMessage && received > 0) {
                try {
                    m_onMessage(buffer.data(), received);
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "sockpp: message callback exception: %s\n", e.what());
                } catch (...) {
                    std::fprintf(stderr, "sockpp: message callback unknown exception\n");
                }
            }
        } else {
            // Socket was closed or an error occurred.
            m_connected = false;
            if (m_onDisconnected) {
                try { m_onDisconnected(); } catch (...) {}
            }

            if (m_autoReconnect && !m_stopRequested) {
                selector.remove(m_socket);
                if (tryReconnect()) {
                    // Reconnection succeeded; re-add the new socket and resume.
                    selector.add(m_socket);
                    continue;
                }
            }
            break;
        }
    }
}

bool TcpClient::tryReconnect() {
    // Snapshot reconnect interval under the lock to avoid a data race with
    // setAutoReconnect().  The loop condition uses atomics so it needs no lock.
    std::chrono::milliseconds interval;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        interval = m_reconnectInterval;
    }

    while (m_running && m_autoReconnect && !m_connected && !m_stopRequested) {
        std::this_thread::sleep_for(interval);
        if (m_stopRequested) break;

        // Create a fresh socket for the new connection attempt.
        m_socket = TcpSocket();
        const auto status = m_socket.connect(m_serverAddress, m_serverPort, m_timeout);

        if (status == Socket::Status::Done) {
            m_connected = true;
            if (m_onConnected) {
                try { m_onConnected(); } catch (...) {}
            }
            return true;
        }
    }
    return false;
}

} // namespace sockpp
