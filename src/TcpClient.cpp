/**
 * @file TcpClient.cpp
 * @brief High-level TCP client implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpClient.h>

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
  if (m_connected) {
    disconnect();
  }

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

  m_socket.disconnect();

  if (m_receiveThread.joinable()) {
    m_receiveThread.join();
  }
}

bool TcpClient::isConnected() const { return m_connected; }

bool TcpClient::send(const void* data, std::size_t size) {
  if (!m_connected) {
    return false;
  }

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

  while (m_running) {
    std::size_t received = 0;
    Socket::Status status;

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      status = m_socket.receive(buffer.data(), buffer.size(), received);
    }

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
        tryReconnect();
      } else {
        break;
      }
    } else if (status == Socket::Status::Error) {
      if (m_onError) {
        m_onError("Socket error occurred");
      }
      break;
    }
    // Status::NotReady is normal for non-blocking, just continue.
  }
}

void TcpClient::tryReconnect() {
  while (m_running && m_autoReconnect && !m_connected) {
    std::this_thread::sleep_for(m_reconnectInterval);

    if (!m_running) {
      break;
    }

    // Create a new socket for reconnection.
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
