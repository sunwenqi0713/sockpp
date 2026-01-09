/**
 * @file Ftp.cpp
 * @brief FTP client implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/Ftp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>

#include "SocketImpl.h"

namespace sockpp {

// Ftp::Response implementation.

Ftp::Response::Response(Status code, std::string message) : m_status(code), m_message(std::move(message)) {}

bool Ftp::Response::isOk() const { return static_cast<int>(m_status) < 400; }

Ftp::Response::Status Ftp::Response::getStatus() const { return m_status; }

const std::string& Ftp::Response::getMessage() const { return m_message; }

// Ftp::DirectoryResponse implementation.

Ftp::DirectoryResponse::DirectoryResponse(const Response& response) : Response(response) {
  if (isOk()) {
    // Extract the directory from the server response.
    const std::string::size_type begin = getMessage().find('"');
    const std::string::size_type end = getMessage().rfind('"');
    if ((begin != std::string::npos) && (end != std::string::npos) && (begin != end)) {
      m_directory = getMessage().substr(begin + 1, end - begin - 1);
    }
  }
}

const std::filesystem::path& Ftp::DirectoryResponse::getDirectory() const { return m_directory; }

// Ftp::ListingResponse implementation.

Ftp::ListingResponse::ListingResponse(const Response& response, std::string data) : Response(response) {
  if (isOk()) {
    // Fill the array of strings.
    std::string::size_type last_pos = 0;
    for (std::string::size_type pos = data.find("\r\n"); pos != std::string::npos; pos = data.find("\r\n", last_pos)) {
      m_listing.emplace_back(data.substr(last_pos, pos - last_pos));
      last_pos = pos + 2;
    }
  }
}

const std::vector<std::filesystem::path>& Ftp::ListingResponse::getListing() const { return m_listing; }

// Ftp::DataChannel implementation.

class Ftp::DataChannel {
 public:
  explicit DataChannel(Ftp& owner) : m_ftp(owner) {}

  Response open(TransferMode mode) {
    // Open a data connection in passive mode.
    Response response = m_ftp.sendCommand("PASV");
    if (response.isOk()) {
      // Extract the connection address and port from the response.
      const std::string::size_type begin = response.getMessage().find('(');
      const std::string::size_type end = response.getMessage().find(')');
      if ((begin == std::string::npos) || (end == std::string::npos)) {
        return Response(Response::Status::InvalidResponse);
      }

      // Parse the numbers in the brackets.
      std::array<int, 6> data{};
      std::size_t index = 0;
      const std::string::size_type data_end = end - begin - 1;
      for (std::string::size_type pos = 0; pos < data_end && index < 6; ++index) {
        std::string::size_type comma_pos = response.getMessage().find(',', begin + pos + 1);
        if (comma_pos == std::string::npos || comma_pos > end) {
          comma_pos = end;
        }
        data[index] = std::stoi(response.getMessage().substr(begin + pos + 1, comma_pos - begin - pos - 1));
        pos = comma_pos - begin;
      }

      // Reconstruct IP and port.
      const auto ip = IpAddress(static_cast<std::uint8_t>(data[0]), static_cast<std::uint8_t>(data[1]),
                                static_cast<std::uint8_t>(data[2]), static_cast<std::uint8_t>(data[3]));
      const auto port = static_cast<unsigned short>(data[4] * 256 + data[5]);

      // Connect the data socket.
      if (m_dataSocket.connect(ip, port) != Socket::Status::Done) {
        return Response(Response::Status::ConnectionFailed);
      }
    }

    // Set the mode (Ascii, Binary, Ebcdic).
    if (response.isOk()) {
      switch (mode) {
        case TransferMode::Binary:
          response = m_ftp.sendCommand("TYPE", "I");
          break;
        case TransferMode::Ascii:
          response = m_ftp.sendCommand("TYPE", "A");
          break;
        case TransferMode::Ebcdic:
          response = m_ftp.sendCommand("TYPE", "E");
          break;
      }
    }

    return response;
  }

  void receive(std::ostream& stream) {
    std::array<char, 1024> buffer{};
    std::size_t received = 0;
    while (m_dataSocket.receive(buffer.data(), buffer.size(), received) == Socket::Status::Done) {
      stream.write(buffer.data(), static_cast<std::streamsize>(received));
      if (!stream) {
        break;
      }
    }
    m_dataSocket.disconnect();
  }

  void send(std::istream& stream) {
    std::array<char, 1024> buffer{};
    std::size_t count = 0;

    while (!stream.eof()) {
      stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      if (stream) {
        count = buffer.size();
      } else {
        count = static_cast<std::size_t>(stream.gcount());
      }

      if (count > 0) {
        if (m_dataSocket.send(buffer.data(), count) != Socket::Status::Done) {
          break;
        }
      }
    }
    m_dataSocket.disconnect();
  }

 private:
  Ftp& m_ftp;
  TcpSocket m_dataSocket;
};

// Ftp implementation.

Ftp::~Ftp() { (void)disconnect(); }

Ftp::Response Ftp::connect(IpAddress server, unsigned short port, std::chrono::milliseconds timeout) {
  // Connect to the server.
  if (m_commandSocket.connect(server, port, timeout) != Socket::Status::Done) {
    return Response(Response::Status::ConnectionFailed);
  }

  // Get the response to the connection.
  return getResponse();
}

Ftp::Response Ftp::login() { return login("anonymous", "user@sockpp.org"); }

Ftp::Response Ftp::login(const std::string& name, const std::string& password) {
  Response response = sendCommand("USER", name);
  if (response.isOk()) {
    response = sendCommand("PASS", password);
  }

  return response;
}

Ftp::Response Ftp::disconnect() {
  Response response = sendCommand("QUIT");
  if (response.isOk()) {
    m_commandSocket.disconnect();
  }

  return response;
}

Ftp::Response Ftp::keepAlive() { return sendCommand("NOOP"); }

Ftp::DirectoryResponse Ftp::getWorkingDirectory() { return sendCommand("PWD"); }

Ftp::ListingResponse Ftp::getDirectoryListing(const std::string& directory) {
  // Open a data channel on default port (passive mode).
  std::ostringstream directory_data;
  DataChannel data(*this);
  Response response = data.open(TransferMode::Ascii);

  if (response.isOk()) {
    // Tell the server to send us the listing.
    response = sendCommand("NLST", directory);
    if (response.isOk()) {
      data.receive(directory_data);
      response = getResponse();
    }
  }

  return {response, directory_data.str()};
}

Ftp::Response Ftp::changeDirectory(const std::string& directory) { return sendCommand("CWD", directory); }

Ftp::Response Ftp::parentDirectory() { return sendCommand("CDUP"); }

Ftp::Response Ftp::createDirectory(const std::string& name) { return sendCommand("MKD", name); }

Ftp::Response Ftp::deleteDirectory(const std::string& name) { return sendCommand("RMD", name); }

Ftp::Response Ftp::renameFile(const std::string& file, const std::string& newName) {
  Response response = sendCommand("RNFR", file);
  if (response.isOk()) {
    response = sendCommand("RNTO", newName);
  }

  return response;
}

Ftp::Response Ftp::deleteFile(const std::string& name) { return sendCommand("DELE", name); }

Ftp::Response Ftp::download(const std::string& remoteFile, const std::string& localPath, TransferMode mode) {
  // Open a data channel using the given transfer mode.
  DataChannel data(*this);
  Response response = data.open(mode);

  if (response.isOk()) {
    // Tell the server to start the transfer.
    response = sendCommand("RETR", remoteFile);
    if (response.isOk()) {
      // Extract the filename from the file path.
      const std::filesystem::path filepath(remoteFile);
      const std::filesystem::path filename = filepath.filename();
      const std::filesystem::path local_filepath = std::filesystem::path(localPath) / filename;

      // Create the file and truncate it if necessary.
      std::ofstream file(local_filepath, std::ios_base::binary | std::ios_base::trunc);
      if (!file) {
        return Response(Response::Status::InvalidFile);
      }

      // Receive the file data.
      data.receive(file);

      // Close the file.
      file.close();

      // Get the response from the server.
      response = getResponse();

      // If the download was unsuccessful, delete the partial file.
      if (!response.isOk()) {
        std::filesystem::remove(local_filepath);
      }
    }
  }

  return response;
}

Ftp::Response Ftp::upload(const std::string& localFile, const std::string& remotePath, TransferMode mode, bool append) {
  // Open a data channel using the given transfer mode.
  DataChannel data(*this);
  Response response = data.open(mode);

  if (response.isOk()) {
    // Open the file.
    std::ifstream file(localFile, std::ios_base::binary);
    if (!file) {
      return Response(Response::Status::InvalidFile);
    }

    // Extract the filename from the file path.
    const std::filesystem::path filepath(localFile);
    std::string filename = filepath.filename().string();

    // Make sure the remote path ends with '/'.
    std::string path = remotePath;
    if (!path.empty() && (path[path.size() - 1] != '/')) {
      path += '/';
    }

    // Tell the server to start the transfer.
    response = sendCommand(append ? "APPE" : "STOR", path + filename);
    if (response.isOk()) {
      // Send the file data.
      data.send(file);

      // Get the response from the server.
      response = getResponse();
    }
  }

  return response;
}

Ftp::Response Ftp::sendCommand(const std::string& command, const std::string& parameter) {
  // Build the command string.
  std::string command_str;
  if (!parameter.empty()) {
    command_str = command + " " + parameter + "\r\n";
  } else {
    command_str = command + "\r\n";
  }

  // Send it to the server.
  if (m_commandSocket.send(command_str.c_str(), command_str.size()) != Socket::Status::Done) {
    return Response(Response::Status::ConnectionClosed);
  }

  // Get the response.
  return getResponse();
}

Ftp::Response Ftp::getResponse() {
  // We'll use a selection of the socket to wait for data
  // (otherwise a receive would hang forever).
  std::array<char, 1024> buffer{};
  while (true) {
    // Receive data from the server.
    std::size_t length = 0;
    const Socket::Status status = m_commandSocket.receive(buffer.data(), buffer.size(), length);
    if (status != Socket::Status::Done) {
      if (status == Socket::Status::Disconnected) {
        return Response(Response::Status::ConnectionClosed);
      }
      return Response(Response::Status::InvalidResponse);
    }

    // There can be multiple responses in the buffer.
    m_receiveBuffer.append(buffer.data(), length);

    // Check if we have a complete response.
    std::size_t pos = 0;
    while (pos < m_receiveBuffer.size()) {
      // Find the end of line.
      std::size_t end_pos = m_receiveBuffer.find("\r\n", pos);
      if (end_pos == std::string::npos) {
        break;
      }

      // Extract the line.
      std::string line = m_receiveBuffer.substr(pos, end_pos - pos);
      pos = end_pos + 2;

      // Check if this is the last line.
      if (line.size() >= 4 && line[3] == ' ') {
        // This is the last line.
        Response::Status response_status = Response::Status::InvalidResponse;
        if (std::isdigit(line[0]) && std::isdigit(line[1]) && std::isdigit(line[2])) {
          response_status = static_cast<Response::Status>(std::stoi(line.substr(0, 3)));
        }

        // Remove the processed data from the buffer.
        m_receiveBuffer.erase(0, pos);

        return Response(response_status, line.size() > 4 ? line.substr(4) : "");
      }
    }
  }
}

}  // namespace sockpp
