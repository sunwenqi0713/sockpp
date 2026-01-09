/**
 * @file echo_server.cpp
 * @brief Echo server example.
 *
 * A simple TCP echo server that accepts multiple clients
 * and echoes back any data received.
 */

#include <array>
#include <iostream>
#include <list>

#include "sockpp.h"

int main() {
  const unsigned short kPort = 55001;

  // Create the TCP listener.
  sockpp::TcpListener listener;

  // Start listening.
  if (listener.listen(kPort) != sockpp::Socket::Status::Done) {
    std::cerr << "Error: Failed to listen on port " << kPort << std::endl;
    return 1;
  }

  std::cout << "Echo server started on port " << kPort << std::endl;
  std::cout << "Press Ctrl+C to stop..." << std::endl;

  // Create a socket selector for handling multiple clients.
  sockpp::SocketSelector selector;
  selector.add(listener);

  // List of connected clients.
  std::list<sockpp::TcpSocket> clients;

  // Main server loop.
  while (true) {
    // Wait for activity on any socket.
    if (selector.wait(std::chrono::milliseconds(100))) {
      // Check if there's a new connection.
      if (selector.isReady(listener)) {
        clients.emplace_back();
        sockpp::TcpSocket& new_client = clients.back();

        if (listener.accept(new_client) == sockpp::Socket::Status::Done) {
          std::cout << "New client connected: " << new_client.getRemoteAddress().value() << ":"
                    << new_client.getRemotePort() << std::endl;

          selector.add(new_client);
        } else {
          clients.pop_back();
        }
      }

      // Check all clients for incoming data.
      for (auto it = clients.begin(); it != clients.end();) {
        sockpp::TcpSocket& client = *it;

        if (selector.isReady(client)) {
          std::array<char, 1024> buffer{};
          std::size_t received = 0;

          sockpp::Socket::Status status = client.receive(buffer.data(), buffer.size(), received);

          if (status == sockpp::Socket::Status::Done) {
            // Echo the data back.
            std::cout << "Received " << received << " bytes from " << client.getRemoteAddress().value() << std::endl;

            (void)client.send(buffer.data(), received);
            ++it;
          } else if (status == sockpp::Socket::Status::Disconnected) {
            std::cout << "Client disconnected: " << client.getRemoteAddress().value() << std::endl;

            selector.remove(client);
            it = clients.erase(it);
          } else {
            ++it;
          }
        } else {
          ++it;
        }
      }
    }
  }

  return 0;
}
