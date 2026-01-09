/**
 * @file IpAddress.h
 * @brief IPv4 address encapsulation.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace sockpp {

/**
 * @brief Encapsulate an IPv4 network address.
 */
class SOCKPP_API IpAddress {
 public:
  /**
   * @brief Default constructor.
   *
   * This constructor creates an empty (invalid) address.
   */
  IpAddress() = default;

  /**
   * @brief Construct the address from a 32-bits integer.
   *
   * This constructor uses the internal representation of
   * the address directly. It should be used for optimization
   * purposes, and only if you got that representation from
   * IpAddress::toInteger().
   *
   * @param address 4 bytes of the address packed into a 32-bits integer.
   * @see toInteger
   */
  explicit IpAddress(std::uint32_t address);

  /**
   * @brief Construct the address from 4 bytes.
   *
   * Calling IpAddress(a, b, c, d) is equivalent to calling
   * IpAddress::resolve("a.b.c.d"), but safer as it doesn't
   * have to parse a string to get the address components.
   *
   * @param byte0 First byte of the address.
   * @param byte1 Second byte of the address.
   * @param byte2 Third byte of the address.
   * @param byte3 Fourth byte of the address.
   */
  IpAddress(std::uint8_t byte0, std::uint8_t byte1, std::uint8_t byte2, std::uint8_t byte3);

  /**
   * @brief Create an address from a string.
   *
   * Here @a address can be either a decimal address
   * (ex: "192.168.1.56") or a network name (ex: "localhost").
   *
   * @param address IP address or network name.
   * @return Address if provided argument was valid, otherwise `std::nullopt`.
   */
  [[nodiscard]] static std::optional<IpAddress> resolve(std::string_view address);

  /**
   * @brief Get a string representation of the address.
   *
   * The returned string is the decimal representation of the
   * IP address (like "192.168.1.56"), even if it was constructed
   * from a host name.
   *
   * @return String representation of the address.
   * @see toInteger
   */
  [[nodiscard]] std::string toString() const;

  /**
   * @brief Get an integer representation of the address.
   *
   * The returned number is the internal representation of the
   * address, and should be used for optimization purposes only
   * (like sending the address through a socket).
   * The integer produced by this function can then be converted
   * back to an IpAddress with the proper constructor.
   *
   * @return 32-bits unsigned integer representation of the address.
   * @see toString
   */
  [[nodiscard]] std::uint32_t toInteger() const;

  /**
   * @brief Get the computer's local address.
   *
   * The local address is the address of the computer from the
   * LAN point of view, i.e. something like 192.168.1.56. It is
   * meaningful only for communications over the local network.
   *
   * @return Local address of the computer.
   * @see getPublicAddress
   */
  [[nodiscard]] static std::optional<IpAddress> getLocalAddress();

  /**
   * @brief Get the computer's public address.
   *
   * The public address is the address of the computer from the
   * internet point of view, i.e. something like 89.54.1.169.
   * It is necessary for communications over the world wide web.
   *
   * @param timeout Maximum time to wait.
   * @return Public IP address of the computer.
   * @see getLocalAddress
   */
  [[nodiscard]] static std::optional<IpAddress> getPublicAddress(
      std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

  // NOLINTBEGIN(readability-identifier-naming)
  static const IpAddress Any;        ///< Value representing any address (0.0.0.0).
  static const IpAddress LocalHost;  ///< The "localhost" address (127.0.0.1).
  static const IpAddress Broadcast;  ///< The "broadcast" address (255.255.255.255).
                                     // NOLINTEND(readability-identifier-naming)

 private:
  friend SOCKPP_API bool operator<(IpAddress left, IpAddress right);

  std::uint32_t m_address{};  ///< Address stored as an unsigned 32 bits integer.
  bool m_valid{};             ///< Is the address valid?
};

/**
 * @brief Overload of == operator to compare two IP addresses.
 */
[[nodiscard]] SOCKPP_API bool operator==(IpAddress left, IpAddress right);

/**
 * @brief Overload of != operator to compare two IP addresses.
 */
[[nodiscard]] SOCKPP_API bool operator!=(IpAddress left, IpAddress right);

/**
 * @brief Overload of < operator to compare two IP addresses.
 */
[[nodiscard]] SOCKPP_API bool operator<(IpAddress left, IpAddress right);

/**
 * @brief Overload of > operator to compare two IP addresses.
 */
[[nodiscard]] SOCKPP_API bool operator>(IpAddress left, IpAddress right);

/**
 * @brief Overload of <= operator to compare two IP addresses.
 */
[[nodiscard]] SOCKPP_API bool operator<=(IpAddress left, IpAddress right);

/**
 * @brief Overload of >= operator to compare two IP addresses.
 */
[[nodiscard]] SOCKPP_API bool operator>=(IpAddress left, IpAddress right);

/**
 * @brief Overload of >> operator to extract an IP address from an input stream.
 */
SOCKPP_API std::istream& operator>>(std::istream& stream, std::optional<IpAddress>& address);

/**
 * @brief Overload of << operator to print an IP address to an output stream.
 */
SOCKPP_API std::ostream& operator<<(std::ostream& stream, IpAddress address);

}  // namespace sockpp
