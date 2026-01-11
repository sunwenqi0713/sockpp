/**
 * @file simple_server.cpp
 * @brief Example of using TcpServer wrapper.
 */

#include <sockpp.h>

#include <csignal>
#include <iostream>

namespace {
sockpp::TcpServer* g_server = nullptr;

void signalHandler(int) {
  if (g_server) {
    g_server->stop();
  }
}
}  // namespace

int main() {
  sockpp::TcpServer server;
  g_server = &server;

  // Handle Ctrl+C.
  std::signal(SIGINT, signalHandler);

  // Set up callbacks.
  server.onConnection([](sockpp::TcpServer::ClientId id, sockpp::IpAddress addr) {
    std::cout << "[+] Client " << id << " connected from " << addr << "\n";
  });

  server.onMessage([&server](sockpp::TcpServer::ClientId id, const void* data, std::size_t size) {
    std::string message(static_cast<const char*>(data), size);
    std::cout << "[" << id << "] Received: " << message << "\n";

    // Echo back with prefix.
    std::string response = "Echo: " + message;
    server.send(id, response.data(), response.size());
  });

  server.onDisconnection([](sockpp::TcpServer::ClientId id) { std::cout << "[-] Client " << id << " disconnected\n"; });

  // Start server.
  constexpr unsigned short kPort = 8080;
  if (!server.start(kPort)) {
    std::cerr << "Failed to start server on port " << kPort << "\n";
    return 1;
  }

  std::cout << "Server started on port " << kPort << "\n";
  std::cout << "Press Ctrl+C to stop...\n";

  // Wait until server stops.
  while (server.isRunning()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << "Server stopped.\n";
  return 0;
}
