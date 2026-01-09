/**
 * @file TcpListener.h
 * @brief TCP listener socket implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>
#include <sockpp/IpAddress.h>
#include <sockpp/Socket.h>

namespace sockpp {

class TcpSocket;

/**
 * @brief Socket that listens to new TCP connections.
 */
class SOCKPP_API TcpListener : public Socket {
 public:
  /**
   * @brief Default constructor.
   */
  TcpListener();

  /**
   * @brief Get the port to which the socket is bound locally.
   *
   * If the socket is not listening to a port, this function
   * returns 0.
   *
   * @return Port to which the socket is bound.
   * @see listen
   */
  [[nodiscard]] unsigned short getLocalPort() const;

  /**
   * @brief Start listening for incoming connection attempts.
   *
   * This function makes the socket start listening on the
   * specified port, waiting for incoming connection attempts.
   *
   * If the socket is already listening on a port when this
   * function is called, it will stop listening on the old
   * port before starting to listen on the new port.
   *
   * When providing sockpp::Socket::AnyPort as port, the listener
   * will request an available port from the system.
   * The chosen port can be retrieved by calling getLocalPort().
   *
   * @param port Port to listen on for incoming connection attempts.
   * @param address Address of the interface to listen on.
   * @return Status code.
   * @see accept, close
   */
  [[nodiscard]] Status listen(unsigned short port, IpAddress address = IpAddress::Any);

  /**
   * @brief Stop listening and close the socket.
   *
   * This function gracefully stops the listener. If the
   * socket is not listening, this function has no effect.
   *
   * @see listen
   */
  void close();

  /**
   * @brief Accept a new connection.
   *
   * If the socket is in blocking mode, this function will
   * not return until a connection is actually received.
   *
   * @param socket Socket that will hold the new connection.
   * @return Status code.
   * @see listen
   */
  [[nodiscard]] Status accept(TcpSocket& socket);
};

}  // namespace sockpp
