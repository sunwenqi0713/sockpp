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
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace sockpp {

/**
 * @brief High-level TCP client with callback-based event handling.
 *
 * Features:
 *   - Non-blocking connect with timeout
 *   - Callback-based message, connect, disconnect, and error events
 *   - Automatic reconnection with configurable interval
 *   - Thread-safe send()
 *
 * Example usage:
 * @code
 * sockpp::TcpClient client;
 * client.onMessage([](const void* data, std::size_t size) {
 *     std::cout << "Received: " << std::string(static_cast<const char*>(data), size) << "\n";
 * });
 * if (client.connect("127.0.0.1", 8080)) {
 *     client.send("Hello!");
 * }
 * @endcode
 */
class SOCKPP_API TcpClient {
public:
    using ConnectedCallback = std::function<void()>;
    using MessageCallback = std::function<void(const void*, std::size_t)>;
    using DisconnectedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string&)>;

    TcpClient();
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    /// Set the callback for successful connection.
    void onConnected(ConnectedCallback callback);
    /// Set the callback for received messages.
    void onMessage(MessageCallback callback);
    /// Set the callback for disconnection.
    void onDisconnected(DisconnectedCallback callback);
    /// Set the callback for errors.
    void onError(ErrorCallback callback);

    /// Connect to a server by hostname.
    bool connect(const std::string& host, unsigned short port,
                 std::chrono::milliseconds timeout = std::chrono::seconds(5));
    /// Connect to a server by IP address.
    bool connect(IpAddress address, unsigned short port,
                 std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /// Disconnect from the server.
    void disconnect();
    /// Check if connected.
    [[nodiscard]] bool isConnected() const;

    /// Send a string view.
    bool send(std::string_view data);
    /// Send raw data.
    bool send(const void* data, std::size_t size);

    /// Get the local port number, or 0 if not connected.
    [[nodiscard]] unsigned short getLocalPort() const;
    /// Get the remote address, or nullopt if not connected.
    [[nodiscard]] std::optional<IpAddress> getRemoteAddress() const;
    /// Get the remote port, or 0 if not connected.
    [[nodiscard]] unsigned short getRemotePort() const;

    /// Enable or disable automatic reconnection.
    void setAutoReconnect(bool enable, std::chrono::milliseconds interval = std::chrono::seconds(3));

private:
    void receiveLoop();
    bool tryReconnect();
    void invokeError(const std::string& msg);

    TcpSocket m_socket;
    std::thread m_receiveThread;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    IpAddress m_serverAddress;
    unsigned short m_serverPort{0};
    std::chrono::milliseconds m_timeout{std::chrono::seconds(5)};

    std::atomic<bool> m_autoReconnect{false};
    std::chrono::milliseconds m_reconnectInterval{std::chrono::seconds(3)};
    mutable std::mutex m_mutex;

    ConnectedCallback m_onConnected;
    MessageCallback m_onMessage;
    DisconnectedCallback m_onDisconnected;
    ErrorCallback m_onError;
};

} // namespace sockpp
