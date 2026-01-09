/**
 * @file Socket.h
 * @brief Base socket class definition.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>
#include <sockpp/SocketHandle.h>

namespace sockpp {

/**
 * @brief Base class for all the socket types.
 */
class SOCKPP_API Socket {
 public:
  /**
   * @brief Status codes that may be returned by socket functions.
   */
  enum class Status {
    Done,          ///< The socket has sent / received the data.
    NotReady,      ///< The socket is not ready to send / receive data yet.
    Partial,       ///< The socket sent a part of the data.
    Disconnected,  ///< The TCP socket has been disconnected.
    Error          ///< An unexpected error happened.
  };

  // NOLINTNEXTLINE(readability-identifier-naming)
  /// Special value that tells the system to pick any available port.
  static constexpr unsigned short AnyPort{0};

  /**
   * @brief Destructor.
   */
  virtual ~Socket();

  /**
   * @brief Deleted copy constructor.
   */
  Socket(const Socket&) = delete;

  /**
   * @brief Deleted copy assignment.
   */
  Socket& operator=(const Socket&) = delete;

  /**
   * @brief Move constructor.
   */
  Socket(Socket&& socket) noexcept;

  /**
   * @brief Move assignment.
   */
  Socket& operator=(Socket&& socket) noexcept;

  /**
   * @brief Set the blocking state of the socket.
   *
   * In blocking mode, calls will not return until they have
   * completed their task. For example, a call to Receive in
   * blocking mode won't return until some data was actually
   * received.
   * In non-blocking mode, calls will always return immediately,
   * using the return code to signal whether there was data
   * available or not.
   * By default, all sockets are blocking.
   *
   * @param blocking `true` to set the socket as blocking, `false` for
   * non-blocking.
   *
   * @see isBlocking
   */
  void setBlocking(bool blocking);

  /**
   * @brief Tell whether the socket is in blocking or non-blocking mode.
   * @return `true` if the socket is blocking, `false` otherwise.
   * @see setBlocking
   */
  [[nodiscard]] bool isBlocking() const;

 protected:
  /**
   * @brief Types of protocols that the socket can use.
   */
  enum class Type {
    Tcp,  ///< TCP protocol.
    Udp   ///< UDP protocol.
  };

  /**
   * @brief Default constructor.
   *
   * This constructor can only be accessed by derived classes.
   *
   * @param type Type of the socket (TCP or UDP).
   */
  explicit Socket(Type type);

  /**
   * @brief Return the internal handle of the socket.
   *
   * The returned handle may be invalid if the socket
   * was not created yet (or already destroyed).
   * This function can only be accessed by derived classes.
   *
   * @return The internal (OS-specific) handle of the socket.
   */
  [[nodiscard]] SocketHandle getNativeHandle() const;

  /**
   * @brief Create the internal representation of the socket.
   *
   * This function can only be accessed by derived classes.
   */
  void create();

  /**
   * @brief Create the internal representation of the socket from a socket
   * handle.
   *
   * This function can only be accessed by derived classes.
   *
   * @param handle OS-specific handle of the socket to wrap.
   */
  void create(SocketHandle handle);

  /**
   * @brief Close the socket gracefully.
   *
   * This function can only be accessed by derived classes.
   */
  void close();

 private:
  friend class SocketSelector;

  Type m_type;              ///< Type of the socket (TCP or UDP).
  SocketHandle m_socket;    ///< Socket descriptor.
  bool m_isBlocking{true};  ///< Current blocking mode of the socket.
};

}  // namespace sockpp
