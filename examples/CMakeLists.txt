############################################################
# sockpp examples
############################################################

# Echo server example
add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server PRIVATE sockpp::sockpp)

# Echo client example
add_executable(echo_client echo_client.cpp)
target_link_libraries(echo_client PRIVATE sockpp::sockpp)

# HTTP client example
add_executable(http_client http_client.cpp)
target_link_libraries(http_client PRIVATE sockpp::sockpp)

# TCP client example (TcpClient wrapper)
add_executable(tcp_client tcp_client.cpp)
target_link_libraries(tcp_client PRIVATE sockpp::sockpp)

# TCP server example (TcpServer wrapper)
add_executable(tcp_server tcp_server.cpp)
target_link_libraries(tcp_server PRIVATE sockpp::sockpp)

# UDP chat example
add_executable(udp_chat udp_chat.cpp udp_wrappers.cpp)
target_link_libraries(udp_chat PRIVATE sockpp::sockpp)