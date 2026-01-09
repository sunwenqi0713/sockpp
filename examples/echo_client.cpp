/**
 * @file echo_client.cpp
 * @brief Echo client example.
 *
 * A simple TCP client that connects to the echo server
 * and sends/receives data.
 */

#include <array>
#include <iostream>
#include <string>

#include "sockpp.h"

int main() {
  // Server address and port.
  const std::string kServerAddress = "127.0.0.1";
  const unsigned short kPort = 55001;

  // Resolve the server address.
  auto address = sockpp::IpAddress::resolve(kServerAddress);
  if (!address.has_value()) {
    std::cerr << "Error: Could not resolve address " << kServerAddress << std::endl;
    return 1;
  }

  // Create a TCP socket.
  sockpp::TcpSocket socket;

  // Connect to the server.
  std::cout << "Connecting to " << kServerAddress << ":" << kPort << "..." << std::endl;

  if (socket.connect(address.value(), kPort, std::chrono::seconds(5)) != sockpp::Socket::Status::Done) {
    std::cerr << "Error: Could not connect to server" << std::endl;
    return 1;
  }

  std::cout << "Connected!" << std::endl;
  std::cout << "Type messages to send (empty line to quit):" << std::endl;

  // Main client loop.
  std::string input;
  while (true) {
    // Read input from user.
    std::cout << "> ";
    std::getline(std::cin, input);

    if (input.empty()) {
      break;
    }

    // Send the message.
    if (socket.send(input.c_str(), input.size()) != sockpp::Socket::Status::Done) {
      std::cerr << "Error: Failed to send data" << std::endl;
      break;
    }

    // Receive the echo.
    std::array<char, 1024> buffer{};
    std::size_t received = 0;

    if (socket.receive(buffer.data(), buffer.size(), received) == sockpp::Socket::Status::Done) {
      std::cout << "Echo: " << std::string(buffer.data(), received) << std::endl;
    } else {
      std::cerr << "Error: Failed to receive data" << std::endl;
      break;
    }
  }

  std::cout << "Disconnecting..." << std::endl;
  socket.disconnect();

  return 0;
}
