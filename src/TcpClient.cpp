/**
 * @file TcpClient.cpp
 * @brief High-level TCP client implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpClient.h>

#include <sockpp/SocketSelector.h>

#include <array>

namespace sockpp {

TcpClient::~TcpClient() { disconnect(); }

void TcpClient::onConnected(ConnectedCallback callback) { m_onConnected = std::move(callback); }

void TcpClient::onMessage(MessageCallback callback) { m_onMessage = std::move(callback); }

void TcpClient::onDisconnected(DisconnectedCallback callback) { m_onDisconnected = std::move(callback); }

void TcpClient::onError(ErrorCallback callback) { m_onError = std::move(callback); }

bool TcpClient::connect(const std::string& host, unsigned short port, std::chrono::milliseconds timeout) {
  auto address = IpAddress::resolve(host);
  if (!address) {
    if (m_onError) {
      m_onError("Failed to resolve host: " + host);
    }
    return false;
  }

  return connect(*address, port, timeout);
}

bool TcpClient::connect(IpAddress address, unsigned short port, std::chrono::milliseconds timeout) {
  // Clean up any previous connection state before starting fresh.
  // disconnect() is idempotent: all operations are no-ops when already disconnected.
  disconnect();

  // Save connection info for potential reconnect.
  m_serverAddress = address;
  m_serverPort = port;
  m_timeout = timeout;

  const auto status = m_socket.connect(address, port, timeout);
  if (status != Socket::Status::Done) {
    if (m_onError) {
      m_onError("Failed to connect to server");
    }
    return false;
  }

  m_connected = true;
  m_running = true;

  // Start receive thread.
  m_receiveThread = std::thread(&TcpClient::receiveLoop, this);

  if (m_onConnected) {
    m_onConnected();
  }

  return true;
}

void TcpClient::disconnect() {
  m_running = false;
  m_connected = false;

  // SocketSelector uses a 100ms timeout, so the receive thread will exit cleanly
  // within one poll cycle once m_running is false. We join first, then close the
  // socket — no need to force-close the fd to unblock recv().
  if (m_receiveThread.joinable()) {
    m_receiveThread.join();
  }

  m_socket.disconnect();
}

bool TcpClient::isConnected() const { return m_connected; }

bool TcpClient::send(const void* data, std::size_t size) {
  if (!m_connected) {
    return false;
  }

  // Serialize concurrent send() callers; recv() runs in its own thread with no mutex.
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_socket.send(data, size) == Socket::Status::Done;
}

bool TcpClient::send(const std::string& message) { return send(message.data(), message.size()); }

unsigned short TcpClient::getLocalPort() const { return m_socket.getLocalPort(); }

std::optional<IpAddress> TcpClient::getRemoteAddress() const { return m_socket.getRemoteAddress(); }

unsigned short TcpClient::getRemotePort() const { return m_socket.getRemotePort(); }

void TcpClient::setAutoReconnect(bool enable, std::chrono::milliseconds interval) {
  m_autoReconnect = enable;
  m_reconnectInterval = interval;
}

void TcpClient::receiveLoop() {
  std::array<char, 4096> buffer{};

  // SocketSelector with a short timeout lets us check m_running periodically
  // without blocking indefinitely — disconnect() can then join cleanly without
  // needing to force-close the socket to interrupt a blocking recv().
  SocketSelector selector;
  selector.add(m_socket);

  while (m_running) {
    if (!selector.wait(std::chrono::milliseconds(100))) {
      // Timeout — no data, loop back to check m_running.
      continue;
    }

    if (!selector.isReady(m_socket)) {
      continue;
    }

    std::size_t received = 0;
    const Socket::Status status = m_socket.receive(buffer.data(), buffer.size(), received);

    if (status == Socket::Status::Done) {
      if (m_onMessage && received > 0) {
        m_onMessage(buffer.data(), received);
      }
    } else if (status == Socket::Status::Disconnected) {
      m_connected = false;

      if (m_onDisconnected) {
        m_onDisconnected();
      }

      if (m_autoReconnect && m_running) {
        selector.remove(m_socket);
        tryReconnect();
        if (m_connected) {
          selector.add(m_socket);
        }
      } else {
        break;
      }
    } else if (status == Socket::Status::Error) {
      // Only report an error if we are not shutting down; a force-close from
      // disconnect() would otherwise trigger a spurious error callback.
      if (m_onError && m_running) {
        m_onError("Socket error occurred");
      }
      break;
    }
  }
}

void TcpClient::tryReconnect() {
  while (m_running && m_autoReconnect && !m_connected) {
    // Sleep in small increments so that a disconnect() call can interrupt
    // quickly instead of waiting for the full reconnect interval.
    constexpr auto kStep = std::chrono::milliseconds(100);
    for (auto elapsed = std::chrono::milliseconds(0);
         elapsed < m_reconnectInterval && m_running;
         elapsed += kStep) {
      std::this_thread::sleep_for(kStep);
    }

    if (!m_running) {
      break;
    }

    // Create a fresh socket for the new connection attempt.
    m_socket = TcpSocket();

    const auto status = m_socket.connect(m_serverAddress, m_serverPort, m_timeout);
    if (status == Socket::Status::Done) {
      m_connected = true;

      if (m_onConnected) {
        m_onConnected();
      }
      break;
    }
  }
}

}  // namespace sockpp
