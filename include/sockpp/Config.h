/**
 * @file Config.h
 * @brief Platform detection and configuration macros.
 *
 * sockpp - Simple C++ Socket Library
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * zlib/libpng License
 */

#pragma once

// sockpp version.
#define SOCKPP_VERSION_MAJOR 1
#define SOCKPP_VERSION_MINOR 0
#define SOCKPP_VERSION_PATCH 0

// Identify the operating system.
#if defined(_WIN32)

// Windows.
#define SOCKPP_SYSTEM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif

#elif defined(__APPLE__) && defined(__MACH__)

#include <TargetConditionals.h>

#if TARGET_OS_MAC
// macOS.
#define SOCKPP_SYSTEM_MACOS
#else
#error "Unsupported Apple platform"
#endif

#elif defined(__unix__)

#if defined(__linux__)
// Linux.
#define SOCKPP_SYSTEM_LINUX
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
// FreeBSD.
#define SOCKPP_SYSTEM_FREEBSD
#elif defined(__OpenBSD__)
// OpenBSD.
#define SOCKPP_SYSTEM_OPENBSD
#elif defined(__NetBSD__)
// NetBSD.
#define SOCKPP_SYSTEM_NETBSD
#else
#error "Unsupported UNIX platform"
#endif

#else
#error "Unsupported operating system"
#endif

// Portable import / export macros.
#if !defined(SOCKPP_STATIC)

#if defined(SOCKPP_SYSTEM_WINDOWS)
// Windows compilers need specific keywords for export and import.
#define SOCKPP_API_EXPORT __declspec(dllexport)
#define SOCKPP_API_IMPORT __declspec(dllimport)

#ifdef _MSC_VER
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif

#else
// GCC/Clang.
#define SOCKPP_API_EXPORT __attribute__((__visibility__("default")))
#define SOCKPP_API_IMPORT __attribute__((__visibility__("default")))
#endif

#else
// Static build doesn't need import/export macros.
#define SOCKPP_API_EXPORT
#define SOCKPP_API_IMPORT
#endif

// Define portable import / export macros.
#if defined(SOCKPP_EXPORTS)
#define SOCKPP_API SOCKPP_API_EXPORT
#else
#define SOCKPP_API SOCKPP_API_IMPORT
#endif
