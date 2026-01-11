/**
 * @file TcpClient.h
 * @brief High-level TCP client wrapper.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>
#include <sockpp/IpAddress.h>
#include <sockpp/TcpSocket.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace sockpp {

/**
 * @brief High-level TCP client with callback-based event handling.
 *
 * Example usage:
 * @code
 * sockpp::TcpClient client;
 *
 * client.onConnected([]() {
 *     std::cout << "Connected to server!\n";
 * });
 *
 * client.onMessage([](const void* data, std::size_t size) {
 *     std::cout << "Received: " << std::string(static_cast<const char*>(data), size) << "\n";
 * });
 *
 * client.onDisconnected([]() {
 *     std::cout << "Disconnected from server\n";
 * });
 *
 * if (client.connect("127.0.0.1", 8080)) {
 *     client.send("Hello, Server!", 14);
 * }
 * @endcode
 */
class SOCKPP_API TcpClient {
 public:
  /// Callback type for successful connection.
  using ConnectedCallback = std::function<void()>;

  /// Callback type for received messages.
  using MessageCallback = std::function<void(const void*, std::size_t)>;

  /// Callback type for disconnection.
  using DisconnectedCallback = std::function<void()>;

  /// Callback type for connection errors.
  using ErrorCallback = std::function<void(const std::string&)>;

  /**
   * @brief Default constructor.
   */
  TcpClient() = default;

  /**
   * @brief Destructor. Disconnects if connected.
   */
  ~TcpClient();

  // Non-copyable.
  TcpClient(const TcpClient&) = delete;
  TcpClient& operator=(const TcpClient&) = delete;

  // Movable.
  TcpClient(TcpClient&&) noexcept = default;
  TcpClient& operator=(TcpClient&&) noexcept = default;

  /**
   * @brief Set the callback for successful connection.
   * @param callback Function to call when connected.
   */
  void onConnected(ConnectedCallback callback);

  /**
   * @brief Set the callback for received messages.
   * @param callback Function to call when data is received.
   */
  void onMessage(MessageCallback callback);

  /**
   * @brief Set the callback for disconnection.
   * @param callback Function to call when disconnected.
   */
  void onDisconnected(DisconnectedCallback callback);

  /**
   * @brief Set the callback for errors.
   * @param callback Function to call on error.
   */
  void onError(ErrorCallback callback);

  /**
   * @brief Connect to a server.
   * @param host Host address (IP or hostname).
   * @param port Port number.
   * @param timeout Connection timeout.
   * @return true if connection was successful.
   */
  bool connect(const std::string& host, unsigned short port,
               std::chrono::milliseconds timeout = std::chrono::seconds(5));

  /**
   * @brief Connect to a server.
   * @param address Server IP address.
   * @param port Port number.
   * @param timeout Connection timeout.
   * @return true if connection was successful.
   */
  bool connect(IpAddress address, unsigned short port, std::chrono::milliseconds timeout = std::chrono::seconds(5));

  /**
   * @brief Disconnect from the server.
   */
  void disconnect();

  /**
   * @brief Check if connected to a server.
   * @return true if connected.
   */
  [[nodiscard]] bool isConnected() const;

  /**
   * @brief Send data to the server.
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   * @return true if send was successful.
   */
  bool send(const void* data, std::size_t size);

  /**
   * @brief Send a string to the server.
   * @param message String to send.
   * @return true if send was successful.
   */
  bool send(const std::string& message);

  /**
   * @brief Get the local port.
   * @return Local port number, or 0 if not connected.
   */
  [[nodiscard]] unsigned short getLocalPort() const;

  /**
   * @brief Get the remote address.
   * @return Remote address, or nullopt if not connected.
   */
  [[nodiscard]] std::optional<IpAddress> getRemoteAddress() const;

  /**
   * @brief Get the remote port.
   * @return Remote port number, or 0 if not connected.
   */
  [[nodiscard]] unsigned short getRemotePort() const;

  /**
   * @brief Enable or disable auto-reconnection.
   * @param enable Whether to enable auto-reconnect.
   * @param interval Interval between reconnection attempts.
   */
  void setAutoReconnect(bool enable, std::chrono::milliseconds interval = std::chrono::seconds(3));

 private:
  void receiveLoop();
  void tryReconnect();

  TcpSocket m_socket;
  std::thread m_receiveThread;
  std::atomic<bool> m_connected{false};
  std::atomic<bool> m_running{false};

  // Connection info for reconnect.
  IpAddress m_serverAddress;
  unsigned short m_serverPort{0};
  std::chrono::milliseconds m_timeout{std::chrono::seconds(5)};

  // Auto-reconnect settings.
  std::atomic<bool> m_autoReconnect{false};
  std::chrono::milliseconds m_reconnectInterval{std::chrono::seconds(3)};

  // Callbacks.
  ConnectedCallback m_onConnected;
  MessageCallback m_onMessage;
  DisconnectedCallback m_onDisconnected;
  ErrorCallback m_onError;

  mutable std::mutex m_mutex;
};

}  // namespace sockpp
