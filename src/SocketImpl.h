/**
 * @file SocketImpl.h
 * @brief Platform-specific socket implementation helper.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Socket.h>
#include <sockpp/SocketHandle.h>

#if defined(SOCKPP_SYSTEM_WINDOWS)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>

#endif

#include <cstdint>

namespace sockpp::detail {

/**
 * @brief Helper class implementing all the non-portable socket stuff.
 */
class SocketImpl {
 public:
#if defined(SOCKPP_SYSTEM_WINDOWS)
  using AddrLength = int;
  using Size = int;
#else
  using AddrLength = socklen_t;
  using Size = std::size_t;
#endif

  /**
   * @brief Create an internal sockaddr_in address.
   * @param address Target address.
   * @param port Target port.
   * @return sockaddr_in ready to be used by socket functions.
   */
  static sockaddr_in createAddress(std::uint32_t address, unsigned short port);

  /**
   * @brief Return the value of the invalid socket.
   * @return Special value of the invalid socket.
   */
  static SocketHandle invalidSocket();

  /**
   * @brief Close and destroy a socket.
   * @param sock Handle of the socket to close.
   */
  static void close(SocketHandle sock);

  /**
   * @brief Set a socket as blocking or non-blocking.
   * @param sock Handle of the socket.
   * @param block New blocking state of the socket.
   */
  static void setBlocking(SocketHandle sock, bool block);

  /**
   * @brief Get the last socket error status.
   * @return Status corresponding to the last socket error.
   */
  static Socket::Status getErrorStatus();
};

}  // namespace sockpp::detail
