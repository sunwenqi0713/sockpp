/**
 * @file udp_example.cpp
 * @brief Example of using Udp wrapper.
 */

#include <iostream>

#include "udp_wrappers.h"

int main() {
  UdpSender sender;

  // Set default target.
  if (!sender.setTarget("127.0.0.1", 9000)) {
    std::cerr << "Failed to set target address.\n";
    return 1;
  }

  std::cout << "UDP Sender ready. Sending to 127.0.0.1:9000\n";
  std::cout << "Type messages to send (empty line to quit):\n";

  // Also set up a receiver to get replies.
  UdpReceiver replyReceiver;
  replyReceiver.onMessage([](const void* data, std::size_t size, sockpp::IpAddress sender, unsigned short port) {
    std::string message(static_cast<const char*>(data), size);
    std::cout << "Reply from " << sender << ":" << port << " - " << message << "\n";
  });

  // Start on any available port.
  replyReceiver.start(sockpp::Socket::AnyPort);

  // Read and send messages.
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) {
      break;
    }

    if (!sender.send(line)) {
      std::cerr << "Failed to send message.\n";
    } else {
      std::cout << "Sent: " << line << "\n";
    }

    // Give time to receive reply.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  replyReceiver.stop();
  return 0;
}
