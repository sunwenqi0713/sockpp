#include "udp_wrappers.h"

#include <array>

// UdpSender 实现
UdpSender::UdpSender(sockpp::IpAddress address, unsigned short port) : m_targetAddress(address), m_targetPort(port) {}

UdpSender::UdpSender(const std::string& host, unsigned short port) : m_targetPort(port) {
  if (auto addr = sockpp::IpAddress::resolve(host)) {
    m_targetAddress = *addr;
  }
}

void UdpSender::setTarget(sockpp::IpAddress address, unsigned short port) {
  m_targetAddress = address;
  m_targetPort = port;
}

bool UdpSender::setTarget(const std::string& host, unsigned short port) {
  auto addr = sockpp::IpAddress::resolve(host);
  if (!addr) {
    return false;
  }
  m_targetAddress = *addr;
  m_targetPort = port;
  return true;
}

bool UdpSender::send(const void* data, std::size_t size) {
  if (m_targetPort == 0) {
    return false;
  }
  return m_socket.send(data, size, m_targetAddress, m_targetPort) == sockpp::Socket::Status::Done;
}

bool UdpSender::send(const std::string& message) { return send(message.data(), message.size()); }

bool UdpSender::send(const void* data, std::size_t size, sockpp::IpAddress address, unsigned short port) {
  return m_socket.send(data, size, address, port) == sockpp::Socket::Status::Done;
}

bool UdpSender::send(const std::string& message, sockpp::IpAddress address, unsigned short port) {
  return send(message.data(), message.size(), address, port);
}

bool UdpSender::send(const std::string& message, const std::string& host, unsigned short port) {
  auto addr = sockpp::IpAddress::resolve(host);
  if (!addr) {
    return false;
  }
  return send(message.data(), message.size(), *addr, port);
}

bool UdpSender::broadcast(const void* data, std::size_t size, unsigned short port) {
  return m_socket.send(data, size, sockpp::IpAddress::Broadcast, port) == sockpp::Socket::Status::Done;
}

bool UdpSender::broadcast(const std::string& message, unsigned short port) {
  return broadcast(message.data(), message.size(), port);
}

unsigned short UdpSender::getLocalPort() const { return m_socket.getLocalPort(); }

// UdpReceiver 实现
UdpReceiver::~UdpReceiver() { stop(); }

void UdpReceiver::onMessage(MessageCallback callback) { m_onMessage = std::move(callback); }

void UdpReceiver::onError(ErrorCallback callback) { m_onError = std::move(callback); }

bool UdpReceiver::start(unsigned short port, sockpp::IpAddress address) {
  if (m_running) {
    return false;
  }

  if (m_socket.bind(port, address) != sockpp::Socket::Status::Done) {
    if (m_onError) {
      m_onError("Failed to bind to port " + std::to_string(port));
    }
    return false;
  }

  m_running = true;
  m_thread = std::thread(&UdpReceiver::receiveLoop, this);

  return true;
}

void UdpReceiver::stop() {
  if (!m_running) {
    return;
  }

  m_running = false;
  m_socket.unbind();

  if (m_thread.joinable()) {
    m_thread.join();
  }
}

bool UdpReceiver::isRunning() const { return m_running; }

unsigned short UdpReceiver::getLocalPort() const { return m_socket.getLocalPort(); }

bool UdpReceiver::reply(const void* data, std::size_t size, sockpp::IpAddress address, unsigned short port) {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_socket.send(data, size, address, port) == sockpp::Socket::Status::Done;
}

bool UdpReceiver::reply(const std::string& message, sockpp::IpAddress address, unsigned short port) {
  return reply(message.data(), message.size(), address, port);
}

void UdpReceiver::receiveLoop() {
  std::array<char, sockpp::UdpSocket::MaxDatagramSize> buffer{};

  while (m_running) {
    std::size_t received = 0;
    std::optional<sockpp::IpAddress> senderAddress;
    unsigned short senderPort = 0;

    sockpp::Socket::Status status;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      status = m_socket.receive(buffer.data(), buffer.size(), received, senderAddress, senderPort);
    }

    if (status == sockpp::Socket::Status::Done) {
      if (m_onMessage && received > 0 && senderAddress) {
        m_onMessage(buffer.data(), received, *senderAddress, senderPort);
      }
    } else if (status == sockpp::Socket::Status::Error) {
      if (m_onError) {
        m_onError("Socket error occurred");
      }
      break;
    }
    // Status::NotReady is normal, just continue.
  }
}