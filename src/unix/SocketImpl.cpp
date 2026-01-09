/**
 * @file SocketImpl.cpp
 * @brief Unix-specific socket implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include "../SocketImpl.h"

#include <fcntl.h>

#include <cerrno>
#include <cstdint>
#include <iostream>

namespace sockpp::detail {

sockaddr_in SocketImpl::createAddress(std::uint32_t address, unsigned short port) {
  auto addr = sockaddr_in();
  addr.sin_addr.s_addr = htonl(address);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

#if defined(SOCKPP_SYSTEM_MACOS)
  addr.sin_len = sizeof(addr);
#endif

  return addr;
}

SocketHandle SocketImpl::invalidSocket() { return -1; }

void SocketImpl::close(SocketHandle sock) { ::close(sock); }

void SocketImpl::setBlocking(SocketHandle sock, bool block) {
  const int status = fcntl(sock, F_GETFL);
  if (block) {
    if (fcntl(sock, F_SETFL, status & ~O_NONBLOCK) == -1) {
      std::cerr << "Failed to set socket as blocking\n";
    }
  } else {
    if (fcntl(sock, F_SETFL, status | O_NONBLOCK) == -1) {
      std::cerr << "Failed to set socket as non-blocking\n";
    }
  }
}

Socket::Status SocketImpl::getErrorStatus() {
  // The followings are sometimes equal to EWOULDBLOCK,
  // so we have to make a special case for them in order
  // to avoid having double values in the switch case.
  if ((errno == EAGAIN) || (errno == EINPROGRESS)) {
    return Socket::Status::NotReady;
  }

  // clang-format off
  switch (errno) {
    case EWOULDBLOCK:  return Socket::Status::NotReady;
    case ECONNABORTED: return Socket::Status::Disconnected;
    case ECONNRESET:   return Socket::Status::Disconnected;
    case ETIMEDOUT:    return Socket::Status::Disconnected;
    case ENETRESET:    return Socket::Status::Disconnected;
    case ENOTCONN:     return Socket::Status::Disconnected;
    case EPIPE:        return Socket::Status::Disconnected;
    default:           return Socket::Status::Error;
  }
  // clang-format on
}

}  // namespace sockpp::detail
