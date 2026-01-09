/**
 * @file Socket.cpp
 * @brief Base socket class implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/Socket.h>

#include <iostream>
#include <utility>

#include "SocketImpl.h"

namespace sockpp {

Socket::Socket(Type type) : m_type(type), m_socket(detail::SocketImpl::invalidSocket()) {}

Socket::~Socket() {
  // Close the socket before it gets destructed.
  close();
}

Socket::Socket(Socket&& socket) noexcept
    : m_type(socket.m_type),
      m_socket(std::exchange(socket.m_socket, detail::SocketImpl::invalidSocket())),
      m_isBlocking(socket.m_isBlocking) {}

Socket& Socket::operator=(Socket&& socket) noexcept {
  if (this != &socket) {
    // Close current socket.
    close();

    // Move from other socket.
    m_type = socket.m_type;
    m_socket = std::exchange(socket.m_socket, detail::SocketImpl::invalidSocket());
    m_isBlocking = socket.m_isBlocking;
  }

  return *this;
}

void Socket::setBlocking(bool blocking) {
  // Apply if the socket is already created.
  if (m_socket != detail::SocketImpl::invalidSocket()) {
    detail::SocketImpl::setBlocking(m_socket, blocking);
  }

  m_isBlocking = blocking;
}

bool Socket::isBlocking() const { return m_isBlocking; }

SocketHandle Socket::getNativeHandle() const { return m_socket; }

void Socket::create() {
  // Don't create the socket if it already exists.
  if (m_socket == detail::SocketImpl::invalidSocket()) {
    const SocketHandle handle = socket(PF_INET, m_type == Type::Tcp ? SOCK_STREAM : SOCK_DGRAM, 0);

    if (handle == detail::SocketImpl::invalidSocket()) {
      std::cerr << "Failed to create socket" << std::endl;
      return;
    }

    create(handle);
  }
}

void Socket::create(SocketHandle handle) {
  // Don't create the socket if it already exists.
  if (m_socket == detail::SocketImpl::invalidSocket()) {
    // Assign the new handle.
    m_socket = handle;

    // Set the current blocking state.
    setBlocking(m_isBlocking);

    if (m_type == Type::Tcp) {
      // Disable the Nagle algorithm (i.e. removes buffering of TCP packets).
      int yes = 1;
      if (setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1) {
        std::cerr << "Failed to set socket option \"TCP_NODELAY\"; "
                  << "all your TCP packets will be buffered" << std::endl;
      }

// On Mac OS, disable the SIGPIPE signal on disconnection.
#ifdef SOCKPP_SYSTEM_MACOS
      if (setsockopt(m_socket, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1) {
        std::cerr << "Failed to set socket option \"SO_NOSIGPIPE\"" << std::endl;
      }
#endif
    } else {
      // Enable broadcast by default for UDP sockets.
      int yes = 1;
      if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1) {
        std::cerr << "Failed to enable broadcast on UDP socket" << std::endl;
      }
    }
  }
}

void Socket::close() {
  // Close the socket.
  if (m_socket != detail::SocketImpl::invalidSocket()) {
    detail::SocketImpl::close(m_socket);
    m_socket = detail::SocketImpl::invalidSocket();
  }
}

}  // namespace sockpp
