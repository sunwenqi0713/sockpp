/**
 * @file SocketSelector.h
 * @brief Socket multiplexer for handling multiple sockets.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>

#include <chrono>
#include <memory>

namespace sockpp {

class Socket;

/**
 * @brief Multiplexer that allows to read from multiple sockets.
 */
class SOCKPP_API SocketSelector {
 public:
  /**
   * @brief Default constructor.
   */
  SocketSelector();

  /**
   * @brief Destructor.
   */
  ~SocketSelector();

  /**
   * @brief Copy constructor.
   * @param copy Instance to copy.
   */
  SocketSelector(const SocketSelector& copy);

  /**
   * @brief Overload of assignment operator.
   * @param right Instance to assign.
   * @return Reference to self.
   */
  SocketSelector& operator=(const SocketSelector& right);

  /**
   * @brief Move constructor.
   */
  SocketSelector(SocketSelector&&) noexcept;

  /**
   * @brief Move assignment.
   */
  SocketSelector& operator=(SocketSelector&&) noexcept;

  /**
   * @brief Add a new socket to the selector.
   *
   * This function keeps a weak reference to the socket,
   * so you have to make sure that the socket is not destroyed
   * while it is stored in the selector.
   * This function does nothing if the socket is not valid.
   *
   * @param socket Reference to the socket to add.
   * @see remove, clear
   */
  void add(Socket& socket);

  /**
   * @brief Remove a socket from the selector.
   *
   * This function doesn't destroy the socket, it simply
   * removes the reference that the selector has to it.
   *
   * @param socket Reference to the socket to remove.
   * @see add, clear
   */
  void remove(Socket& socket);

  /**
   * @brief Remove all the sockets stored in the selector.
   *
   * This function doesn't destroy any instance, it simply
   * removes all the references that the selector has to
   * external sockets.
   *
   * @see add, remove
   */
  void clear();

  /**
   * @brief Wait until one or more sockets are ready to receive.
   *
   * This function returns as soon as at least one socket has
   * some data available to be received. To know which sockets are
   * ready, use the isReady function.
   * If you use a timeout and no socket is ready before the timeout
   * is over, the function returns false.
   *
   * @param timeout Maximum time to wait, (use zero for infinity).
   * @return true if there are sockets ready, false otherwise.
   * @see isReady
   */
  [[nodiscard]] bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

  /**
   * @brief Test a socket to know if it is ready to receive data.
   *
   * This function must be used after a call to Wait, to know
   * which sockets are ready to receive data. If a socket is
   * ready, a call to receive will never block because we know
   * that there is data available to read.
   * Note that if this function returns true for a TcpListener,
   * this means that it is ready to accept a new connection.
   *
   * @param socket Socket to test.
   * @return true if the socket is ready to read, false otherwise.
   * @see isReady
   */
  [[nodiscard]] bool isReady(Socket& socket) const;

 private:
  struct SocketSelectorImpl;

  /// Opaque pointer to the implementation.
  std::unique_ptr<SocketSelectorImpl> m_impl;
};

}  // namespace sockpp
