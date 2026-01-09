/**
 * @file SocketHandle.h
 * @brief Platform-specific socket handle type definition.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>

#if defined(SOCKPP_SYSTEM_WINDOWS)
#include <basetsd.h>
#endif

namespace sockpp {

// Define the low-level socket handle type, specific to each platform.
#if defined(SOCKPP_SYSTEM_WINDOWS)

using SocketHandle = UINT_PTR;

#else

using SocketHandle = int;

#endif

}  // namespace sockpp