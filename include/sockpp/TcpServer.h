/**
 * @file TcpServer.h
 * @brief High-level TCP server wrapper.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>
#include <sockpp/IpAddress.h>
#include <sockpp/SocketSelector.h>
#include <sockpp/TcpListener.h>
#include <sockpp/TcpSocket.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace sockpp {

/**
 * @brief High-level TCP server with callback-based event handling.
 *
 * Example usage:
 * @code
 * sockpp::TcpServer server;
 *
 * server.onConnection([](sockpp::TcpServer::ClientId id, sockpp::IpAddress addr) {
 *     std::cout << "Client connected: " << id << " from " << addr << "\n";
 * });
 *
 * server.onMessage([&](sockpp::TcpServer::ClientId id, const void* data, std::size_t size) {
 *     // Echo back
 *     server.send(id, data, size);
 * });
 *
 * server.onDisconnection([](sockpp::TcpServer::ClientId id) {
 *     std::cout << "Client disconnected: " << id << "\n";
 * });
 *
 * server.start(8080);
 * @endcode
 */
class SOCKPP_API TcpServer {
 public:
  /// Unique identifier for connected clients.
  using ClientId = std::uint64_t;

  /// Callback type for new connections.
  using ConnectionCallback = std::function<void(ClientId, IpAddress)>;

  /// Callback type for received messages.
  using MessageCallback = std::function<void(ClientId, const void*, std::size_t)>;

  /// Callback type for client disconnections.
  using DisconnectionCallback = std::function<void(ClientId)>;

  /**
   * @brief Default constructor.
   */
  TcpServer() = default;

  /**
   * @brief Destructor. Stops the server if running.
   */
  ~TcpServer();

  // Non-copyable.
  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;

  // Movable.
  TcpServer(TcpServer&&) noexcept = default;
  TcpServer& operator=(TcpServer&&) noexcept = default;

  /**
   * @brief Set the callback for new client connections.
   * @param callback Function to call when a client connects.
   */
  void onConnection(ConnectionCallback callback);

  /**
   * @brief Set the callback for received messages.
   * @param callback Function to call when data is received.
   */
  void onMessage(MessageCallback callback);

  /**
   * @brief Set the callback for client disconnections.
   * @param callback Function to call when a client disconnects.
   */
  void onDisconnection(DisconnectionCallback callback);

  /**
   * @brief Start the server on the specified port.
   * @param port Port to listen on.
   * @param address Address to bind to (default: any).
   * @return true if server started successfully.
   */
  bool start(unsigned short port, IpAddress address = IpAddress::Any);

  /**
   * @brief Stop the server.
   */
  void stop();

  /**
   * @brief Check if the server is running.
   * @return true if the server is running.
   */
  [[nodiscard]] bool isRunning() const;

  /**
   * @brief Send data to a specific client.
   * @param clientId Target client ID.
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   * @return true if send was successful.
   */
  bool send(ClientId clientId, const void* data, std::size_t size);

  /**
   * @brief Send data to all connected clients.
   * @param data Pointer to data to send.
   * @param size Size of data in bytes.
   */
  void broadcast(const void* data, std::size_t size);

  /**
   * @brief Disconnect a specific client.
   * @param clientId Client to disconnect.
   */
  void disconnect(ClientId clientId);

  /**
   * @brief Get the number of connected clients.
   * @return Number of connected clients.
   */
  [[nodiscard]] std::size_t clientCount() const;

 private:
  void serverLoop();

  struct ClientInfo {
    TcpSocket socket;
    IpAddress address;
  };

  TcpListener m_listener;
  SocketSelector m_selector;
  std::unordered_map<ClientId, ClientInfo> m_clients;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  std::atomic<ClientId> m_nextClientId{1};

  ConnectionCallback m_onConnection;
  MessageCallback m_onMessage;
  DisconnectionCallback m_onDisconnection;

  mutable std::mutex m_mutex;
};

}  // namespace sockpp
