/**
 * @file TcpServer.cpp
 * @brief High-level TCP server implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpServer.h>

#include <array>

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

  std::lock_guard<std::mutex> lock(m_mutex);

  // Disconnect all clients.
  for (auto& [id, client] : m_clients) {
    m_selector.remove(client.socket);
  }
  m_clients.clear();

  m_selector.remove(m_listener);
  m_listener.close();
}

bool TcpServer::isRunning() const { return m_running; }

bool TcpServer::send(ClientId clientId, const void* data, std::size_t size) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_clients.find(clientId);
  if (it == m_clients.end()) {
    return false;
  }

  return it->second.socket.send(data, size) == Socket::Status::Done;
}

void TcpServer::broadcast(const void* data, std::size_t size) {
  std::lock_guard<std::mutex> lock(m_mutex);

  for (auto& [id, client] : m_clients) {
    (void)client.socket.send(data, size);
  }
}

void TcpServer::disconnect(ClientId clientId) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_clients.find(clientId);
  if (it != m_clients.end()) {
    m_selector.remove(it->second.socket);
    m_clients.erase(it);

    if (m_onDisconnection) {
      m_onDisconnection(clientId);
    }
  }
}

std::size_t TcpServer::clientCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_clients.size();
}

void TcpServer::serverLoop() {
  constexpr auto kTimeout = std::chrono::milliseconds(100);
  std::array<char, 4096> buffer{};

  while (m_running) {
    if (m_selector.wait(kTimeout)) {
      // Check for new connections.
      if (m_selector.isReady(m_listener)) {
        TcpSocket client;
        if (m_listener.accept(client) == Socket::Status::Done) {
          const ClientId clientId = m_nextClientId++;
          const auto address = client.getRemoteAddress().value_or(IpAddress::Any);

          {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_selector.add(client);
            m_clients[clientId] = ClientInfo{std::move(client), address};
          }

          if (m_onConnection) {
            m_onConnection(clientId, address);
          }
        }
      }

      // Check for data from clients.
      std::vector<ClientId> toRemove;

      {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& [id, client] : m_clients) {
          if (m_selector.isReady(client.socket)) {
            std::size_t received = 0;
            const auto status = client.socket.receive(buffer.data(), buffer.size(), received);

            if (status == Socket::Status::Done) {
              if (m_onMessage) {
                m_onMessage(id, buffer.data(), received);
              }
            } else if (status == Socket::Status::Disconnected) {
              toRemove.push_back(id);
            }
          }
        }
      }

      // Remove disconnected clients.
      for (const auto id : toRemove) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_clients.find(id);
        if (it != m_clients.end()) {
          m_selector.remove(it->second.socket);
          m_clients.erase(it);
        }

        if (m_onDisconnection) {
          m_onDisconnection(id);
        }
      }
    }
  }
}

}  // namespace sockpp
