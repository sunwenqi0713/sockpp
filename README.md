# sockpp

**Simple C++ Socket Library**

A lightweight, cross-platform C++17 networking library.

## Features

- 🔌 **TCP/UDP Sockets** - Full-featured TCP and UDP socket support
- 📦 **Packet Serialization** - Automatic byte-order handling for network data
- 🌐 **HTTP Client** - Simple HTTP/1.x client
- 📂 **FTP Client** - Basic FTP client functionality
- 🔄 **Socket Selector** - Multiplexing for handling multiple sockets
- 💻 **Cross-Platform** - Windows, Linux, macOS, BSD

## Requirements

- C++17 compatible compiler
- CMake 3.16+

## Building

```bash
# Clone the repository
git clone https://github.com/sunwenqi0713/sockpp.git
cd sockpp

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Install (optional)
cmake --install . --prefix /usr/local
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `SOCKPP_BUILD_SHARED` | ON | Build shared library |
| `SOCKPP_BUILD_EXAMPLES` | ON | Build example programs |

## Quick Start

### TCP Server

```cpp
#include <sockpp/sockpp.h>
#include <iostream>

int main()
{
  sockpp::TcpListener listener;

  if (listener.listen(8080) != sockpp::Socket::Status::Done)
  {
    std::cerr << "Failed to listen on port 8080\n";
    return 1;
  }

  std::cout << "Server listening on port 8080\n";

  sockpp::TcpSocket client;
  if (listener.accept(client) == sockpp::Socket::Status::Done)
  {
    std::cout << "Client connected: " << client.getRemoteAddress().value() << "\n";

    // Send/receive data...
    char buffer[1024];
    std::size_t received;
    client.receive(buffer, sizeof(buffer), received);
  }

  return 0;
}
```

### TCP Client

```cpp
#include <sockpp/sockpp.h>
#include <iostream>

int main()
{
  sockpp::TcpSocket socket;

  auto address = sockpp::IpAddress::resolve("127.0.0.1");
  if (!address)
  {
    std::cerr << "Invalid address\n";
    return 1;
  }

  if (socket.connect(*address, 8080) == sockpp::Socket::Status::Done)
  {
    std::cout << "Connected!\n";

    // Send data
    const char* message = "Hello, server!";
    socket.send(message, strlen(message));
  }

  return 0;
}
```

### Using Packets

```cpp
#include <sockpp/sockpp.h>

// Sending structured data
sockpp::Packet packet;
packet << "Hello" << 42 << 3.14f;
socket.send(packet);

// Receiving structured data
sockpp::Packet received;
socket.receive(received);

std::string str;
int num;
float pi;
received >> str >> num >> pi;
```

### HTTP Client

```cpp
#include <sockpp/sockpp.h>
#include <iostream>

int main()
{
  sockpp::Http http("http://example.com");
  sockpp::Http::Request request("/");

  auto response = http.sendRequest(request);

  std::cout << "Status: " << static_cast<int>(response.getStatus()) << "\n";
  std::cout << "Body: " << response.getBody() << "\n";

  return 0;
}
```

### Multi-Client Server with Selector

```cpp
#include <sockpp/sockpp.h>
#include <list>

int main()
{
    sockpp::TcpListener listener;
    listener.listen(8080);

    sockpp::SocketSelector selector;
    selector.add(listener);

    std::list<sockpp::TcpSocket> clients;

    while (true)
    {
        if (selector.wait(std::chrono::milliseconds(100)))
        {
            if (selector.isReady(listener))
            {
                // New connection
                clients.emplace_back();
                if (listener.accept(clients.back()) == sockpp::Socket::Status::Done)
                    selector.add(clients.back());
                else
                    clients.pop_back();
            }

            for (auto& client : clients)
            {
                if (selector.isReady(client))
                {
                    // Data available on this client
                    // ... handle data ...
                }
            }
        }
    }
}
```

## API Reference

### Core Classes

| Class | Description |
|-------|-------------|
| `sockpp::TcpSocket` | TCP socket for connection-oriented communication |
| `sockpp::TcpListener` | TCP server socket for accepting connections |
| `sockpp::UdpSocket` | UDP socket for connectionless communication |
| `sockpp::SocketSelector` | Multiplexer for monitoring multiple sockets |
| `sockpp::IpAddress` | IPv4 address representation |
| `sockpp::Packet` | Binary data serialization |
| `sockpp::Http` | HTTP client |
| `sockpp::Ftp` | FTP client |

### Socket Status Codes

```cpp
enum class Status
{
    Done,         // Operation completed successfully
    NotReady,     // Socket not ready (non-blocking)
    Partial,      // Partial send (non-blocking TCP)
    Disconnected, // Connection closed
    Error         // An error occurred
};
```

## License

This software is provided under the zlib/libpng license.

