/**
 * @file http_client.cpp
 * @brief HTTP client example.
 *
 * A simple HTTP client that fetches a web page.
 */

#include <iostream>
#include <string>

#include "sockpp.h"

int main(int argc, char* argv[]) {
  std::string url = "http://example.com";

  if (argc > 1) {
    url = argv[1];
  }

  std::cout << "Fetching: " << url << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  // Create HTTP client.
  sockpp::Http http(url);

  // Create a GET request.
  sockpp::Http::Request request("/", sockpp::Http::Request::Method::Get);

  // Send the request with a 10-second timeout.
  sockpp::Http::Response response = http.sendRequest(request, std::chrono::seconds(10));

  // Check the response status.
  std::cout << "Status: " << static_cast<int>(response.getStatus()) << std::endl;
  std::cout << "HTTP Version: " << response.getMajorHttpVersion() << "." << response.getMinorHttpVersion() << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  // Print headers.
  std::cout << "Content-Type: " << response.getField("content-type") << std::endl;
  std::cout << "Content-Length: " << response.getField("content-length") << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  // Print body (truncated if too long).
  const std::string& body = response.getBody();
  if (body.size() > 500) {
    std::cout << body.substr(0, 500) << std::endl;
    std::cout << "... (truncated, " << body.size() << " bytes total)" << std::endl;
  } else {
    std::cout << body << std::endl;
  }

  return response.getStatus() == sockpp::Http::Response::Status::Ok ? 0 : 1;
}
