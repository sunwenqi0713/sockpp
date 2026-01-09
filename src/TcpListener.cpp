/**
 * @file TcpListener.cpp
 * @brief TCP listener socket implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/TcpListener.h>
#include <sockpp/TcpSocket.h>

#include <iostream>

#include "SocketImpl.h"

namespace sockpp {

TcpListener::TcpListener() : Socket(Type::Tcp) {}

unsigned short TcpListener::getLocalPort() const {
  if (getNativeHandle() != detail::SocketImpl::invalidSocket()) {
    // Retrieve information about the local end of the socket.
    sockaddr_in address{};
    detail::SocketImpl::AddrLength size = sizeof(address);
    if (getsockname(getNativeHandle(), reinterpret_cast<sockaddr*>(&address), &size) != -1) {
      return ntohs(address.sin_port);
    }
  }

  // We failed to retrieve the port.
  return 0;
}

Socket::Status TcpListener::listen(unsigned short port, IpAddress address) {
  // Close the socket if it is already bound.
  close();

  // Create the internal socket if it doesn't exist.
  create();

  // Check if the address is valid.
  if (address == IpAddress::Broadcast) {
    return Status::Error;
  }

  // Bind the socket to the specified port.
  sockaddr_in addr = detail::SocketImpl::createAddress(address.toInteger(), port);
  if (bind(getNativeHandle(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    // Not likely to happen, but...
    std::cerr << "Failed to bind listener socket to port " << port << std::endl;
    return Status::Error;
  }

  // Listen to the bound port.
  if (::listen(getNativeHandle(), SOMAXCONN) == -1) {
    // Oops, socket is deaf.
    std::cerr << "Failed to listen to port " << port << std::endl;
    return Status::Error;
  }

  return Status::Done;
}

void TcpListener::close() {
  // Simply close the socket.
  Socket::close();
}

Socket::Status TcpListener::accept(TcpSocket& socket) {
  // Make sure that we're listening.
  if (getNativeHandle() == detail::SocketImpl::invalidSocket()) {
    std::cerr << "Failed to accept a new connection, the socket is not listening" << std::endl;
    return Status::Error;
  }

  // Accept a new connection.
  sockaddr_in address{};
  detail::SocketImpl::AddrLength length = sizeof(address);
  const SocketHandle remote = ::accept(getNativeHandle(), reinterpret_cast<sockaddr*>(&address), &length);

  // Check for errors.
  if (remote == detail::SocketImpl::invalidSocket()) {
    return detail::SocketImpl::getErrorStatus();
  }

  // Initialize the new connected socket.
  socket.close();
  socket.create(remote);

  return Status::Done;
}

}  // namespace sockpp
