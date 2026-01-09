/**
 * @file Http.cpp
 * @brief HTTP client implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/Http.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <sstream>
#include <utility>

namespace {

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return The lowercase version of the string.
 */
std::string ToLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](char c) { return static_cast<char>(std::tolower(c)); });
  return str;
}

}  // namespace

namespace sockpp {

Http::Request::Request(const std::string& uri, Method method, const std::string& body) : m_method(method) {
  setUri(uri);
  setBody(body);
}

void Http::Request::setField(const std::string& field, const std::string& value) { m_fields[ToLower(field)] = value; }

void Http::Request::setMethod(Http::Request::Method method) { m_method = method; }

void Http::Request::setUri(const std::string& uri) {
  m_uri = uri;

  // Make sure it starts with a '/'.
  if (m_uri.empty() || (m_uri[0] != '/')) {
    m_uri.insert(m_uri.begin(), '/');
  }
}

void Http::Request::setHttpVersion(unsigned int major, unsigned int minor) {
  m_majorVersion = major;
  m_minorVersion = minor;
}

void Http::Request::setBody(const std::string& body) { m_body = body; }

std::string Http::Request::prepare() const {
  std::ostringstream out;

  // Convert the method to its string representation.
  std::string method;
  switch (m_method) {
    case Method::Get:
      method = "GET";
      break;
    case Method::Post:
      method = "POST";
      break;
    case Method::Head:
      method = "HEAD";
      break;
    case Method::Put:
      method = "PUT";
      break;
    case Method::Delete:
      method = "DELETE";
      break;
  }

  // Write the first line containing the request type.
  out << method << " " << m_uri << " ";
  out << "HTTP/" << m_majorVersion << "." << m_minorVersion << "\r\n";

  // Write fields.
  for (const auto& [field_key, field_value] : m_fields) {
    out << field_key << ": " << field_value << "\r\n";
  }

  // Use an extra \r\n to separate the header from the body.
  out << "\r\n";

  // Add the body.
  out << m_body;

  return out.str();
}

bool Http::Request::hasField(const std::string& field) const { return m_fields.find(ToLower(field)) != m_fields.end(); }

const std::string& Http::Response::getField(const std::string& field) const {
  if (const auto it = m_fields.find(ToLower(field)); it != m_fields.end()) {
    return it->second;
  }

  static const std::string empty;
  return empty;
}

Http::Response::Status Http::Response::getStatus() const { return m_status; }

unsigned int Http::Response::getMajorHttpVersion() const { return m_majorVersion; }

unsigned int Http::Response::getMinorHttpVersion() const { return m_minorVersion; }

const std::string& Http::Response::getBody() const { return m_body; }

void Http::Response::parse(const std::string& data) {
  std::istringstream in(data);

  // Extract the HTTP version from the first line.
  std::string version;
  if (in >> version) {
    if ((version.size() >= 8) && (version[6] == '.') && (ToLower(version.substr(0, 5)) == "http/") &&
        std::isdigit(version[5]) && std::isdigit(version[7])) {
      m_majorVersion = static_cast<unsigned int>(version[5] - '0');
      m_minorVersion = static_cast<unsigned int>(version[7] - '0');
    } else {
      // Invalid HTTP version.
      m_status = Status::InvalidResponse;
      return;
    }
  }

  // Extract the status code from the first line.
  int status = 0;
  if (in >> status) {
    m_status = static_cast<Status>(status);
  } else {
    // Invalid status code.
    m_status = Status::InvalidResponse;
    return;
  }

  // Ignore the end of the first line.
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  // Parse the other lines, which contain fields, one by one.
  parseFields(in);

  m_body.clear();

  // Determine whether the transfer is chunked.
  if (ToLower(getField("transfer-encoding")) != "chunked") {
    // Not chunked - just read everything at once.
    std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), std::back_inserter(m_body));
  } else {
    // Chunked - have to read chunk by chunk.
    std::size_t length = 0;

    // Read all chunks, identified by a chunk-size not being 0.
    while (in >> std::hex >> length) {
      // Drop the rest of the line (chunk-extension).
      in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      // Copy the actual content data.
      std::istreambuf_iterator<char> it(in);
      const std::istreambuf_iterator<char> it_end;
      for (std::size_t i = 0; ((i < length) && (it != it_end)); ++i) {
        m_body.push_back(*it);
        ++it;
      }
    }

    // Drop the rest of the line (chunk-extension).
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Read all trailers (if present).
    parseFields(in);
  }
}

void Http::Response::parseFields(std::istream& in) {
  std::string line;
  while (std::getline(in, line) && (line.size() > 2)) {
    const std::string::size_type pos = line.find(": ");
    if (pos != std::string::npos) {
      // Extract the field name and its value.
      const std::string field = line.substr(0, pos);
      std::string value = line.substr(pos + 2);

      // Remove any trailing \r.
      if (!value.empty() && (*value.rbegin() == '\r')) {
        value.erase(value.size() - 1);
      }

      // Add the field.
      m_fields[ToLower(field)] = value;
    }
  }
}

Http::Http(const std::string& host, unsigned short port) { setHost(host, port); }

void Http::setHost(const std::string& host, unsigned short port) {
  // Check the protocol.
  if (ToLower(host.substr(0, 7)) == "http://") {
    // HTTP protocol.
    m_hostName = host.substr(7);
    m_port = (port != 0 ? port : 80);
  } else if (ToLower(host.substr(0, 8)) == "https://") {
    // HTTPS protocol -- unsupported (requires encryption and certificates).
    std::cerr << "HTTPS protocol is not supported by sockpp::Http" << std::endl;
    m_hostName.clear();
    m_port = 0;
  } else {
    // Undefined protocol - use HTTP.
    m_hostName = host;
    m_port = (port != 0 ? port : 80);
  }

  // Remove any trailing '/' from the host name.
  if (!m_hostName.empty() && (*m_hostName.rbegin() == '/')) {
    m_hostName.erase(m_hostName.size() - 1);
  }

  m_host = IpAddress::resolve(m_hostName);
}

Http::Response Http::sendRequest(const Http::Request& request, std::chrono::milliseconds timeout) {
  // First make sure that the request is valid -- add missing mandatory fields.
  Request to_send(request);
  if (!to_send.hasField("From")) {
    to_send.setField("From", "user@sockpp.org");
  }
  if (!to_send.hasField("User-Agent")) {
    to_send.setField("User-Agent", "sockpp/1.x");
  }
  if (!to_send.hasField("Host")) {
    to_send.setField("Host", m_hostName);
  }
  if (!to_send.hasField("Content-Length")) {
    std::ostringstream out;
    out << to_send.m_body.size();
    to_send.setField("Content-Length", out.str());
  }
  if ((to_send.m_method == Request::Method::Post) && !to_send.hasField("Content-Type")) {
    to_send.setField("Content-Type", "application/x-www-form-urlencoded");
  }
  if ((to_send.m_majorVersion * 10 + to_send.m_minorVersion >= 11) && !to_send.hasField("Connection")) {
    to_send.setField("Connection", "close");
  }

  // Prepare the response.
  Response received;

  // Connect the socket to the host.
  if (m_connection.connect(m_host.value(), m_port, timeout) == Socket::Status::Done) {
    // Convert the request to string and send it through the connected socket.
    const std::string request_str = to_send.prepare();

    if (!request_str.empty()) {
      // Send it through the socket.
      if (m_connection.send(request_str.c_str(), request_str.size()) == Socket::Status::Done) {
        // Wait for the server's response.
        std::string received_str;
        std::size_t size = 0;
        std::array<char, 1024> buffer{};
        while (m_connection.receive(buffer.data(), buffer.size(), size) == Socket::Status::Done) {
          received_str.append(buffer.data(), buffer.data() + size);
        }

        // Build the Response object from the received data.
        received.parse(received_str);
      }
    }

    // Close the connection.
    m_connection.disconnect();
  }

  return received;
}

}  // namespace sockpp
