/**
 * @file simple_client.cpp
 * @brief Example of using TcpClient wrapper.
 */

#include <sockpp.h>

#include <iostream>
#include <string>

int main() {
  sockpp::TcpClient client;

  // Set up callbacks.
  client.onConnected([]() { std::cout << "Connected to server!\n"; });

  client.onMessage([](const void* data, std::size_t size) {
    std::string message(static_cast<const char*>(data), size);
    std::cout << "Received: " << message << "\n";
  });

  client.onDisconnected([]() { std::cout << "Disconnected from server.\n"; });

  client.onError([](const std::string& error) { std::cerr << "Error: " << error << "\n"; });

  // Connect to server.
  if (!client.connect("127.0.0.1", 8080)) {
    std::cerr << "Failed to connect to server.\n";
    return 1;
  }

  std::cout << "Type messages to send (empty line to quit):\n";

  // Read and send messages.
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) {
      break;
    }

    if (!client.send(line)) {
      std::cerr << "Failed to send message.\n";
      break;
    }

    // Give time to receive response.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  client.disconnect();
  return 0;
}
