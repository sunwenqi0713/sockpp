#pragma once

#include "sockpp.h"

/**
 * @brief High-level UDP sender for sending datagrams.
 *
 * Example usage:
 * @code
 * UdpSender sender;
 *
 * // Send to a specific address and port.
 * sender.send("Hello, UDP!", "192.168.1.100", 9000);
 *
 * // Or set default target.
 * sender.setTarget("192.168.1.100", 9000);
 * sender.send("Hello again!");
 *
 * // Broadcast.
 * sender.broadcast("Hello everyone!", 9000);
 * @endcode
 */
class UdpSender {
 public:
  /**
   * @brief Default constructor.
   */
  UdpSender() = default;

  /**
   * @brief Constructor with default target.
   * @param address Default target address.
   * @param port Default target port.
   */
  UdpSender(sockpp::IpAddress address, unsigned short port);

  /**
   * @brief Constructor with default target (hostname).
   * @param host Default target hostname or IP string.
   * @param port Default target port.
   */
  UdpSender(const std::string& host, unsigned short port);

  /**
   * @brief Set the default target address and port.
   * @param address Target address.
   * @param port Target port.
   */
  void setTarget(sockpp::IpAddress address, unsigned short port);

  /**
   * @brief Set the default target address and port (hostname).
   * @param host Target hostname or IP string.
   * @param port Target port.
   * @return true if hostname was resolved successfully.
   */
  bool setTarget(const std::string& host, unsigned short port);

  /**
   * @brief Send data to the default target.
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   * @return true if send was successful.
   */
  bool send(const void* data, std::size_t size);

  /**
   * @brief Send a string to the default target.
   * @param message String to send.
   * @return true if send was successful.
   */
  bool send(const std::string& message);

  /**
   * @brief Send data to a specific address and port.
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   * @param address Target address.
   * @param port Target port.
   * @return true if send was successful.
   */
  bool send(const void* data, std::size_t size, sockpp::IpAddress address, unsigned short port);

  /**
   * @brief Send a string to a specific address and port.
   * @param message String to send.
   * @param address Target address.
   * @param port Target port.
   * @return true if send was successful.
   */
  bool send(const std::string& message, sockpp::IpAddress address, unsigned short port);

  /**
   * @brief Send a string to a specific host and port.
   * @param message String to send.
   * @param host Target hostname or IP string.
   * @param port Target port.
   * @return true if send was successful.
   */
  bool send(const std::string& message, const std::string& host, unsigned short port);

  /**
   * @brief Broadcast data to all hosts on the local network.
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   * @param port Target port.
   * @return true if broadcast was successful.
   */
  bool broadcast(const void* data, std::size_t size, unsigned short port);

  /**
   * @brief Broadcast a string to all hosts on the local network.
   * @param message String to send.
   * @param port Target port.
   * @return true if broadcast was successful.
   */
  bool broadcast(const std::string& message, unsigned short port);

  /**
   * @brief Get the local port used for sending.
   * @return Local port number.
   */
  unsigned short getLocalPort() const;

 private:
  sockpp::UdpSocket m_socket;
  sockpp::IpAddress m_targetAddress;
  unsigned short m_targetPort{0};
};

/**
 * @brief High-level UDP receiver with callback-based event handling.
 *
 * Example usage:
 * @code
 * UdpReceiver receiver;
 *
 * receiver.onMessage([](const void* data, std::size_t size,
 *                       sockpp::IpAddress sender, unsigned short port) {
 *     std::string message(static_cast<const char*>(data), size);
 *     std::cout << "From " << sender << ":" << port << " - " << message << "\n";
 * });
 *
 * receiver.start(9000);  // Listen on port 9000
 *
 * // ... do other work ...
 *
 * receiver.stop();
 * @endcode
 */
class UdpReceiver {
 public:
  /// Callback type for received messages.
  using MessageCallback = std::function<void(const void*, std::size_t, sockpp::IpAddress, unsigned short)>;

  /// Callback type for errors.
  using ErrorCallback = std::function<void(const std::string&)>;

  /**
   * @brief Default constructor.
   */
  UdpReceiver() = default;

  /**
   * @brief Destructor. Stops the receiver if running.
   */
  ~UdpReceiver();

  // Non-copyable.
  UdpReceiver(const UdpReceiver&) = delete;
  UdpReceiver& operator=(const UdpReceiver&) = delete;

  // Movable.
  UdpReceiver(UdpReceiver&&) noexcept = default;
  UdpReceiver& operator=(UdpReceiver&&) noexcept = default;

  /**
   * @brief Set the callback for received messages.
   * @param callback Function to call when a datagram is received.
   */
  void onMessage(MessageCallback callback);

  /**
   * @brief Set the callback for errors.
   * @param callback Function to call on error.
   */
  void onError(ErrorCallback callback);

  /**
   * @brief Start receiving on the specified port.
   * @param port Port to listen on.
   * @param address Address to bind to (default: any).
   * @return true if started successfully.
   */
  bool start(unsigned short port, sockpp::IpAddress address = sockpp::IpAddress::Any);

  /**
   * @brief Stop receiving.
   */
  void stop();

  /**
   * @brief Check if the receiver is running.
   * @return true if the receiver is running.
   */
  bool isRunning() const;

  /**
   * @brief Get the local port being listened on.
   * @return Local port number, or 0 if not running.
   */
  unsigned short getLocalPort() const;

  /**
   * @brief Send a reply to a specific address and port.
   *
   * This is useful for sending responses from within the message callback.
   *
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   * @param address Target address.
   * @param port Target port.
   * @return true if send was successful.
   */
  bool reply(const void* data, std::size_t size, sockpp::IpAddress address, unsigned short port);

  /**
   * @brief Send a reply string to a specific address and port.
   * @param message String to send.
   * @param address Target address.
   * @param port Target port.
   * @return true if send was successful.
   */
  bool reply(const std::string& message, sockpp::IpAddress address, unsigned short port);

 private:
  void receiveLoop();

  sockpp::UdpSocket m_socket;
  std::thread m_thread;
  std::atomic<bool> m_running{false};

  MessageCallback m_onMessage;
  ErrorCallback m_onError;

  mutable std::mutex m_mutex;
};