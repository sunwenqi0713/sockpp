/**
 * @file SocketImpl.cpp
 * @brief Windows-specific socket implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include "../SocketImpl.h"

#include <cstdint>

namespace {

/**
 * @brief Windows needs some initialization and cleanup to get sockets working
 * properly... so let's create a class that will do it automatically.
 */
struct SocketInitializer {
  SocketInitializer() {
    WSADATA init;
    WSAStartup(MAKEWORD(2, 2), &init);
  }

  ~SocketInitializer() { WSACleanup(); }
} g_global_initializer;

}  // namespace

namespace sockpp::detail {

sockaddr_in SocketImpl::createAddress(std::uint32_t address, unsigned short port) {
  auto addr = sockaddr_in();
  addr.sin_addr.s_addr = htonl(address);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  return addr;
}

SocketHandle SocketImpl::invalidSocket() { return INVALID_SOCKET; }

void SocketImpl::close(SocketHandle sock) { closesocket(sock); }

void SocketImpl::setBlocking(SocketHandle sock, bool block) {
  u_long blocking = block ? 0 : 1;
  ioctlsocket(sock, static_cast<long>(FIONBIO), &blocking);
}

Socket::Status SocketImpl::getErrorStatus() {
  // clang-format off
  switch (WSAGetLastError()) {
    case WSAEWOULDBLOCK:  return Socket::Status::NotReady;
    case WSAEALREADY:     return Socket::Status::NotReady;
    case WSAECONNABORTED: return Socket::Status::Disconnected;
    case WSAECONNRESET:   return Socket::Status::Disconnected;
    case WSAETIMEDOUT:    return Socket::Status::Disconnected;
    case WSAENETRESET:    return Socket::Status::Disconnected;
    case WSAENOTCONN:     return Socket::Status::Disconnected;
    case WSAEISCONN:      return Socket::Status::Done;  // when connecting a non-blocking socket
    default:              return Socket::Status::Error;
  }
  // clang-format on
}

}  // namespace sockpp::detail
