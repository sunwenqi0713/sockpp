/**
 * @file IpAddress.cpp
 * @brief IPv4 address implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/Http.h>
#include <sockpp/IpAddress.h>

#include <cstring>
#include <iostream>
#include <ostream>

#include "SocketImpl.h"

namespace sockpp {

const IpAddress IpAddress::Any(0, 0, 0, 0);
const IpAddress IpAddress::LocalHost(127, 0, 0, 1);
const IpAddress IpAddress::Broadcast(255, 255, 255, 255);

IpAddress::IpAddress(std::uint32_t address) : m_address(htonl(address)), m_valid(true) {}

IpAddress::IpAddress(std::uint8_t byte0, std::uint8_t byte1, std::uint8_t byte2, std::uint8_t byte3)
    : m_address(htonl(static_cast<std::uint32_t>((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3))),
      m_valid(true) {}

std::optional<IpAddress> IpAddress::resolve(std::string_view address) {
  using namespace std::string_view_literals;

  if (address.empty()) {
    return std::nullopt;
  }

  if (address == "255.255.255.255"sv) {
    // The broadcast address needs to be handled explicitly,
    // because it is also the value returned by inet_addr on error.
    return Broadcast;
  }

  if (address == "0.0.0.0"sv) {
    return Any;
  }

  const std::string address_str(address);
  // Try to convert the address as a byte representation ("xxx.xxx.xxx.xxx").
  struct in_addr addr;
  if (inet_pton(AF_INET, address_str.c_str(), &addr) == 1) {
    return IpAddress(ntohl(addr.s_addr));
  }

  // Not a valid address, try to convert it as a host name.
  addrinfo hints{};
  hints.ai_family = AF_INET;

  addrinfo* result = nullptr;

  if (getaddrinfo(address_str.c_str(), nullptr, &hints, &result) == 0 && result != nullptr) {
    sockaddr_in sin{};
    std::memcpy(&sin, result->ai_addr, sizeof(*result->ai_addr));

    const std::uint32_t ip = sin.sin_addr.s_addr;
    freeaddrinfo(result);

    return IpAddress(ntohl(ip));
  }

  return std::nullopt;
}

std::string IpAddress::toString() const {
  in_addr address{};
  address.s_addr = m_address;
  char buffer[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &address, buffer, sizeof(buffer)) != nullptr) {
    return std::string(buffer);
  }

  return "0.0.0.0";
}

std::uint32_t IpAddress::toInteger() const { return ntohl(m_address); }

std::optional<IpAddress> IpAddress::getLocalAddress() {
  // The method here is to connect a UDP socket to anyone (here to localhost),
  // and get the local socket address with the getsockname function.
  // UDP connection will not send anything to the network, so this function
  // won't cause any overhead.

  // Create the socket.
  const SocketHandle sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock == detail::SocketImpl::invalidSocket()) {
    return std::nullopt;
  }

  // Connect the socket to localhost on any port.
  sockaddr_in address = detail::SocketImpl::createAddress(ntohl(INADDR_LOOPBACK), 9);
  if (connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    detail::SocketImpl::close(sock);
    return std::nullopt;
  }

  // Get the local address of the socket connection.
  detail::SocketImpl::AddrLength size = sizeof(address);
  if (getsockname(sock, reinterpret_cast<sockaddr*>(&address), &size) == -1) {
    detail::SocketImpl::close(sock);
    return std::nullopt;
  }

  // Close the socket.
  detail::SocketImpl::close(sock);

  // Finally build the IP address.
  return IpAddress(ntohl(address.sin_addr.s_addr));
}

std::optional<IpAddress> IpAddress::getPublicAddress(std::chrono::milliseconds timeout) {
  // Get the public IP address from a remote service.
  // The response contains only the IP address.

  Http server("api.ipify.org");
  Http::Request request("/", Http::Request::Method::Get);
  Http::Response page = server.sendRequest(request, timeout);
  if (page.getStatus() == Http::Response::Status::Ok) {
    return IpAddress::resolve(page.getBody());
  }

  return std::nullopt;
}

bool operator==(IpAddress left, IpAddress right) { return !(left < right) && !(right < left); }

bool operator!=(IpAddress left, IpAddress right) { return !(left == right); }

bool operator<(IpAddress left, IpAddress right) {
  return std::make_pair(left.m_valid, left.m_address) < std::make_pair(right.m_valid, right.m_address);
}

bool operator>(IpAddress left, IpAddress right) { return right < left; }

bool operator<=(IpAddress left, IpAddress right) { return !(right < left); }

bool operator>=(IpAddress left, IpAddress right) { return !(left < right); }

std::istream& operator>>(std::istream& stream, std::optional<IpAddress>& address) {
  std::string str;
  stream >> str;
  address = IpAddress::resolve(str);

  return stream;
}

std::ostream& operator<<(std::ostream& stream, IpAddress address) { return stream << address.toString(); }

}  // namespace sockpp
