############################################################
# sockpp - Simple C++ Socket Library
############################################################

cmake_minimum_required(VERSION 3.16)

project(sockpp
    VERSION 1.0.0
    DESCRIPTION "Simple C++ Socket Library"
    LANGUAGES CXX
)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SOCKPP_BUILD_SHARED "Build shared library" ON)
option(SOCKPP_BUILD_EXAMPLES "Build examples" ON)

# Source files
set(SOCKPP_HEADERS
    include/sockpp.h
    include/sockpp/Config.h
    include/sockpp/Ftp.h
    include/sockpp/Http.h
    include/sockpp/IpAddress.h
    include/sockpp/Packet.h
    include/sockpp/Socket.h
    include/sockpp/SocketHandle.h
    include/sockpp/SocketSelector.h
    include/sockpp/TcpListener.h
    include/sockpp/TcpSocket.h
    include/sockpp/UdpSocket.h
)

set(SOCKPP_SOURCES
    src/Ftp.cpp
    src/Http.cpp
    src/IpAddress.cpp
    src/Packet.cpp
    src/Socket.cpp
    src/SocketSelector.cpp
    src/TcpListener.cpp
    src/TcpSocket.cpp
    src/UdpSocket.cpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND SOCKPP_SOURCES src/win32/SocketImpl.cpp)
else()
    list(APPEND SOCKPP_SOURCES src/unix/SocketImpl.cpp)
endif()

# Create library
if(SOCKPP_BUILD_SHARED)
    add_library(sockpp SHARED ${SOCKPP_SOURCES} ${SOCKPP_HEADERS})
    target_compile_definitions(sockpp PRIVATE SOCKPP_EXPORTS)
else()
    add_library(sockpp STATIC ${SOCKPP_SOURCES} ${SOCKPP_HEADERS})
    target_compile_definitions(sockpp PUBLIC SOCKPP_STATIC)
endif()

# Include directories
target_include_directories(sockpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(sockpp PRIVATE ws2_32)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(sockpp PRIVATE /W4)
else()
    target_compile_options(sockpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Alias for modern CMake
add_library(sockpp::sockpp ALIAS sockpp)

# Examples
if(SOCKPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install rules
include(GNUInstallDirs)

install(TARGETS sockpp
    EXPORT sockppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/sockpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT sockppTargets
    FILE sockppTargets.cmake
    NAMESPACE sockpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

# Generate Config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sockppConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sockppConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sockppConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/sockppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/sockppConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

# Summary
message(STATUS "")
message(STATUS "sockpp ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library: ${SOCKPP_BUILD_SHARED}")
message(STATUS "  Examples: ${SOCKPP_BUILD_EXAMPLES}")
message(STATUS "")
