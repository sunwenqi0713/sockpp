/**
 * @file TcpServer.cpp
 * @brief High-level TCP server implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpServer.h>

#include <array>
#include <vector>

namespace sockpp {

TcpServer::~TcpServer() { stop(); }

void TcpServer::onConnection(ConnectionCallback callback) { m_onConnection = std::move(callback); }

void TcpServer::onMessage(MessageCallback callback) { m_onMessage = std::move(callback); }

void TcpServer::onDisconnection(DisconnectionCallback callback) { m_onDisconnection = std::move(callback); }

bool TcpServer::start(unsigned short port, IpAddress address) {
  if (m_running) {
    return false;
  }

  if (m_listener.listen(port, address) != Socket::Status::Done) {
    return false;
  }

  m_selector.add(m_listener);
  m_running = true;
  m_thread = std::thread(&TcpServer::serverLoop, this);

  return true;
}

void TcpServer::stop() {
  if (!m_running) {
    return;
  }

  m_running = false;

  if (m_thread.joinable()) {
    m_thread.join();
  }

  // Collect ids and clean up selector/listener under lock.
  std::vector<ClientId> clientIds;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, client] : m_clients) {
      m_selector.remove(client.socket);
      clientIds.push_back(id);
    }
    m_clients.clear();

    m_selector.remove(m_listener);
    m_listener.close();
  }

  // Fire disconnection callbacks outside the lock.
  if (m_onDisconnection) {
    for (const auto id : clientIds) {
      m_onDisconnection(id);
    }
  }
}

bool TcpServer::isRunning() const { return m_running; }

bool TcpServer::send(ClientId clientId, const void* data, std::size_t size) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_clients.find(clientId);
  if (it == m_clients.end() || it->second.markedForClose) {
    return false;
  }

  return it->second.socket.send(data, size) == Socket::Status::Done;
}

void TcpServer::broadcast(const void* data, std::size_t size) {
  std::lock_guard<std::mutex> lock(m_mutex);

  for (auto& [id, client] : m_clients) {
    if (client.markedForClose) {
      continue;
    }
    if (client.socket.send(data, size) != Socket::Status::Done) {
      // Mark failed sends for cleanup; serverLoop will remove and fire callback.
      client.markedForClose = true;
    }
  }
}

void TcpServer::disconnect(ClientId clientId) {
  // Only mark the client — do NOT touch the selector here. All SocketSelector
  // operations are confined to serverLoop to prevent data races with select().
  bool found = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_clients.find(clientId);
    if (it != m_clients.end() && !it->second.markedForClose) {
      it->second.markedForClose = true;
      found = true;
    }
  }

  if (found && m_onDisconnection) {
    m_onDisconnection(clientId);
  }
}

std::size_t TcpServer::clientCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::size_t count = 0;
  for (const auto& [id, client] : m_clients) {
    if (!client.markedForClose) {
      ++count;
    }
  }
  return count;
}

void TcpServer::serverLoop() {
  constexpr auto kTimeout = std::chrono::milliseconds(100);
  std::array<char, 4096> buffer{};

  // Reuse these across iterations to avoid per-loop heap allocations.
  std::vector<ClientId> toRemove;
  std::vector<std::pair<ClientId, std::vector<char>>> messages;

  while (m_running) {
    // --- Phase 1: drain clients marked for close by the user-facing disconnect().
    //
    // All SocketSelector mutations run exclusively in this thread so that they
    // never race with the select() call below. User threads only set the
    // markedForClose flag (under m_mutex); we do the actual selector.remove()
    // and map erase here, before the next wait().
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      for (auto it = m_clients.begin(); it != m_clients.end();) {
        if (it->second.markedForClose) {
          m_selector.remove(it->second.socket);
          it = m_clients.erase(it);
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
      TcpSocket client;
      if (m_listener.accept(client) == Socket::Status::Done) {
        const ClientId clientId = m_nextClientId++;
        const auto address = client.getRemoteAddress().value_or(IpAddress::Any);

        {
          std::lock_guard<std::mutex> lock(m_mutex);
          m_clients[clientId] = ClientInfo{std::move(client), address};
          m_selector.add(m_clients[clientId].socket);
        }

        if (m_onConnection) {
          m_onConnection(clientId, address);
        }
      }
    }

    // --- Phase 4: receive data from existing clients.
    toRemove.clear();
    messages.clear();

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      for (auto& [id, client] : m_clients) {
        if (client.markedForClose) {
          continue;  // Will be cleaned up at the top of the next iteration.
        }
        if (!m_selector.isReady(client.socket)) {
          continue;
        }

        std::size_t received = 0;
        const auto status = client.socket.receive(buffer.data(), buffer.size(), received);

        if (status == Socket::Status::Done) {
          if (m_onMessage && received > 0) {
            messages.emplace_back(id, std::vector<char>(buffer.data(), buffer.data() + received));
          }
        } else if (status == Socket::Status::Disconnected) {
          toRemove.push_back(id);
        }
      }
    }

    // Invoke message callbacks outside the lock to prevent deadlock when the
    // callback calls send() or other methods that acquire m_mutex.
    for (auto& [id, data] : messages) {
      m_onMessage(id, data.data(), data.size());
    }

    // --- Phase 5: remove clients that disconnected from the remote side.
    for (const auto id : toRemove) {
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_clients.find(id);
        if (it != m_clients.end()) {
          m_selector.remove(it->second.socket);
          m_clients.erase(it);
        }
      }

      if (m_onDisconnection) {
        m_onDisconnection(id);
      }
    }
  }
}

}  // namespace sockpp
