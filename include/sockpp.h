/**
 * @file sockpp.h
 * @brief Main header that includes all sockpp components.
 *
 * sockpp - Simple C++ Socket Library
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * zlib/libpng License
 */

#pragma once

#include <sockpp/Config.h>
#include <sockpp/Ftp.h>
#include <sockpp/Http.h>
#include <sockpp/IpAddress.h>
#include <sockpp/Packet.h>
#include <sockpp/Socket.h>
#include <sockpp/SocketHandle.h>
#include <sockpp/SocketSelector.h>
#include <sockpp/TcpListener.h>
#include <sockpp/TcpSocket.h>
#include <sockpp/UdpSocket.h>

/**
 * @defgroup network Network module
 *
 * Socket-based communication, utilities and higher-level
 * network protocols (HTTP, FTP).
 */
