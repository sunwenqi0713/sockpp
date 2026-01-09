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
