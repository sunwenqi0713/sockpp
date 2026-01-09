/**
 * @file UdpSocket.cpp
 * @brief UDP socket implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/IpAddress.h>
#include <sockpp/Packet.h>
#include <sockpp/UdpSocket.h>

#include <cstring>
#include <iostream>

#include "SocketImpl.h"

namespace sockpp {

UdpSocket::UdpSocket() : Socket(Type::Udp) {}

unsigned short UdpSocket::getLocalPort() const {
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

Socket::Status UdpSocket::bind(unsigned short port, IpAddress address) {
  // Close the socket if it is already bound.
  close();

  // Create the internal socket if it doesn't exist.
  create();

  // Check if the address is valid.
  if (address == IpAddress::Broadcast) {
    return Status::Error;
  }

  // Bind the socket.
  sockaddr_in addr = detail::SocketImpl::createAddress(address.toInteger(), port);
  if (::bind(getNativeHandle(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    std::cerr << "Failed to bind socket to port " << port << std::endl;
    return Status::Error;
  }

  return Status::Done;
}

void UdpSocket::unbind() {
  // Simply close the socket.
  close();
}

Socket::Status UdpSocket::send(const void* data, std::size_t size, IpAddress remoteAddress, unsigned short remotePort) {
  // Create the internal socket if it doesn't exist.
  create();

  // Make sure that all the data will fit in one datagram.
  if (size > MaxDatagramSize) {
    std::cerr << "Cannot send data over the network (the number of bytes to "
              << "send is greater than UdpSocket::MaxDatagramSize)" << std::endl;
    return Status::Error;
  }

  // Build the target address.
  sockaddr_in address = detail::SocketImpl::createAddress(remoteAddress.toInteger(), remotePort);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
  // Send the data (unlike TCP, all the data is always sent in one call).
  const int sent = static_cast<int>(sendto(getNativeHandle(), static_cast<const char*>(data),
                                           static_cast<detail::SocketImpl::Size>(size), 0,
                                           reinterpret_cast<sockaddr*>(&address), sizeof(address)));
#pragma GCC diagnostic pop

  // Check for errors.
  if (sent < 0) {
    return detail::SocketImpl::getErrorStatus();
  }

  return Status::Done;
}

Socket::Status UdpSocket::receive(void* data, std::size_t size, std::size_t& received,
                                  std::optional<IpAddress>& remoteAddress, unsigned short& remotePort) {
  // First clear the variables to fill.
  received = 0;
  remoteAddress = std::nullopt;
  remotePort = 0;

  // Check the destination buffer.
  if (!data) {
    std::cerr << "Cannot receive data from the network "
              << "(the destination buffer is invalid)" << std::endl;
    return Status::Error;
  }

  // Data that will be filled with the other computer's address.
  sockaddr_in address = detail::SocketImpl::createAddress(INADDR_ANY, 0);
  detail::SocketImpl::AddrLength address_size = sizeof(address);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
  // Receive a chunk of bytes.
  const int size_received = static_cast<int>(recvfrom(getNativeHandle(), static_cast<char*>(data),
                                                      static_cast<detail::SocketImpl::Size>(size), 0,
                                                      reinterpret_cast<sockaddr*>(&address), &address_size));
#pragma GCC diagnostic pop

  // Check for errors.
  if (size_received < 0) {
    return detail::SocketImpl::getErrorStatus();
  }

  // Fill the sender information.
  received = static_cast<std::size_t>(size_received);
  remoteAddress = IpAddress(ntohl(address.sin_addr.s_addr));
  remotePort = ntohs(address.sin_port);

  return Status::Done;
}

Socket::Status UdpSocket::send(Packet& packet, IpAddress remoteAddress, unsigned short remotePort) {
  // UDP is a datagram-oriented protocol (as opposed to TCP which is a stream
  // protocol). Sending one datagram is almost safe: it may be lost but if it's
  // received, then its data is guaranteed to be ok. However, splitting a packet
  // into multiple datagrams would be highly unreliable, since datagrams may be
  // reordered, duplicated or lost. That's why sockpp imposes a limit on packet
  // size so that they can be sent in a single datagram.
  // This also removes the overhead associated to packets -- there's no size to
  // send in addition to the packet's data.

  // Get the data to send from the packet.
  std::size_t size = 0;
  const void* data = packet.onSend(size);

  // Send it.
  return send(data, size, remoteAddress, remotePort);
}

Socket::Status UdpSocket::receive(Packet& packet, std::optional<IpAddress>& remoteAddress, unsigned short& remotePort) {
  // See the detailed comment in send(Packet) above.

  // Receive the datagram.
  m_buffer.resize(MaxDatagramSize);
  std::size_t received = 0;
  Status status = receive(m_buffer.data(), m_buffer.size(), received, remoteAddress, remotePort);

  // If we received valid data, we can copy it to the user packet.
  packet.clear();
  if ((status == Status::Done) && (received > 0)) {
    packet.onReceive(m_buffer.data(), received);
  }

  return status;
}

}  // namespace sockpp
