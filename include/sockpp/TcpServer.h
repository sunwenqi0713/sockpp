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
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace sockpp {

/**
 * @brief High-level TCP server with callback-based event handling.
 *
 * Manages multiple client connections in a single acceptor thread using
 * SocketSelector.  Callbacks are invoked from the server thread, so any
 * expensive work should be dispatched to a separate thread pool.
 *
 * Example usage:
 * @code
 * sockpp::TcpServer server;
 * server.onMessage([&](sockpp::TcpServer::ClientId id, const void* data, std::size_t size) {
 *     server.send(id, data, size); // echo
 * });
 * server.start(8080);
 * @endcode
 */
class SOCKPP_API TcpServer {
public:
    using ClientId = std::uint64_t;
    using ConnectionCallback = std::function<void(ClientId, IpAddress)>;
    using MessageCallback = std::function<void(ClientId, const void*, std::size_t)>;
    using DisconnectionCallback = std::function<void(ClientId)>;

    TcpServer();
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    /// Set the callback for new client connections.
    void onConnection(ConnectionCallback callback);
    /// Set the callback for received messages.
    void onMessage(MessageCallback callback);
    /// Set the callback for client disconnections.
    void onDisconnection(DisconnectionCallback callback);

    /// Start the server on the given port (returns false on failure).
    bool start(unsigned short port, IpAddress address = IpAddress::Any);
    /// Stop the server and disconnect all clients.
    void stop();
    /// Check if the server is running.
    [[nodiscard]] bool isRunning() const;

    /// Send data to a specific client.
    bool send(ClientId clientId, const void* data, std::size_t size);
    /// Send data to all connected clients.
    void broadcast(const void* data, std::size_t size);
    /// Gracefully disconnect a specific client.
    void disconnect(ClientId clientId);
    /// Return the number of currently connected clients.
    [[nodiscard]] std::size_t clientCount() const;

private:
    struct ClientInfo {
        TcpSocket socket;
        IpAddress address;
        bool markedForClose{false};
    };

    void serverLoop();

    TcpListener m_listener;
    SocketSelector m_selector;

    /// Protects m_clients and m_on* callbacks.
    mutable std::shared_mutex m_mutex;
    std::unordered_map<ClientId, ClientInfo> m_clients;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<ClientId> m_nextClientId{1};

    ConnectionCallback m_onConnection;
    MessageCallback m_onMessage;
    DisconnectionCallback m_onDisconnection;
};

} // namespace sockpp
