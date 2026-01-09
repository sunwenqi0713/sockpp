/**
 * @file Packet.h
 * @brief Network packet for data serialization.
 *
 * sockpp - Simple C++ Socket Library
 */

#pragma once

#include <sockpp/Config.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sockpp {

/**
 * @brief Utility class to build blocks of data to transfer over the network.
 */
class SOCKPP_API Packet {
 public:
  /**
   * @brief Default constructor.
   *
   * Creates an empty packet.
   */
  Packet() = default;

  /**
   * @brief Virtual destructor.
   */
  virtual ~Packet() = default;

  /**
   * @brief Copy constructor.
   */
  Packet(const Packet&) = default;

  /**
   * @brief Copy assignment.
   */
  Packet& operator=(const Packet&) = default;

  /**
   * @brief Move constructor.
   */
  Packet(Packet&&) noexcept = default;

  /**
   * @brief Move assignment.
   */
  Packet& operator=(Packet&&) noexcept = default;

  /**
   * @brief Append data to the end of the packet.
   * @param data Pointer to the sequence of bytes to append.
   * @param sizeInBytes Number of bytes to append.
   * @see clear
   * @see getReadPosition
   */
  void append(const void* data, std::size_t sizeInBytes);

  /**
   * @brief Get the current reading position in the packet.
   *
   * The next read operation will read data from this position.
   *
   * @return The byte offset of the current read position.
   * @see append
   */
  [[nodiscard]] std::size_t getReadPosition() const;

  /**
   * @brief Clear the packet.
   *
   * After calling Clear, the packet is empty.
   *
   * @see append
   */
  void clear();

  /**
   * @brief Get a pointer to the data contained in the packet.
   *
   * Warning: the returned pointer may become invalid after
   * you append data to the packet, therefore it should never
   * be stored.
   * The return pointer is a nullptr if the packet is empty.
   *
   * @return Pointer to the data.
   * @see getDataSize
   */
  [[nodiscard]] const void* getData() const;

  /**
   * @brief Get the size of the data contained in the packet.
   *
   * This function returns the number of bytes pointed to by
   * what getData returns.
   *
   * @return Data size, in bytes.
   * @see getData
   */
  [[nodiscard]] std::size_t getDataSize() const;

  /**
   * @brief Tell if the reading position has reached the end of the packet.
   *
   * This function is useful to know if there is some data
   * left to be read, without actually reading it.
   *
   * @return true if all data was read, false otherwise.
   * @see operator bool
   */
  [[nodiscard]] bool endOfPacket() const;

  /**
   * @brief Test the validity of the packet, for reading.
   *
   * This operator allows to test the packet as a boolean
   * variable, to check if a reading operation was successful.
   *
   * A packet will be in an invalid state if it has no more
   * data to read.
   *
   * Usage example:
   * @code
   * float x;
   * packet >> x;
   * if (packet) {
   *    // ok, x was extracted successfully
   * }
   * @endcode
   *
   * @return true if last data extraction from packet was successful.
   * @see endOfPacket
   */
  explicit operator bool() const;

  /// Overload of operator>> to read data from the packet.
  Packet& operator>>(bool& data);
  Packet& operator>>(std::int8_t& data);
  Packet& operator>>(std::uint8_t& data);
  Packet& operator>>(std::int16_t& data);
  Packet& operator>>(std::uint16_t& data);
  Packet& operator>>(std::int32_t& data);
  Packet& operator>>(std::uint32_t& data);
  Packet& operator>>(std::int64_t& data);
  Packet& operator>>(std::uint64_t& data);
  Packet& operator>>(float& data);
  Packet& operator>>(double& data);
  Packet& operator>>(char* data);
  Packet& operator>>(std::string& data);
  Packet& operator>>(wchar_t* data);
  Packet& operator>>(std::wstring& data);
  Packet& operator>>(std::u32string& data);

  /// Overload of operator<< to write data into the packet.
  Packet& operator<<(bool data);
  Packet& operator<<(std::int8_t data);
  Packet& operator<<(std::uint8_t data);
  Packet& operator<<(std::int16_t data);
  Packet& operator<<(std::uint16_t data);
  Packet& operator<<(std::int32_t data);
  Packet& operator<<(std::uint32_t data);
  Packet& operator<<(std::int64_t data);
  Packet& operator<<(std::uint64_t data);
  Packet& operator<<(float data);
  Packet& operator<<(double data);
  Packet& operator<<(const char* data);
  Packet& operator<<(const std::string& data);
  Packet& operator<<(const wchar_t* data);
  Packet& operator<<(const std::wstring& data);
  Packet& operator<<(const std::u32string& data);

 protected:
  friend class TcpSocket;
  friend class UdpSocket;

  /**
   * @brief Called before the packet is sent over the network.
   *
   * This function can be defined by derived classes to
   * transform the data before it is sent; this can be
   * used for compression, encryption, etc.
   * The function must return a pointer to the modified data,
   * as well as the number of bytes pointed.
   * The default implementation provides the packet's data
   * without transforming it.
   *
   * @param size Variable to fill with the size of data to send.
   * @return Pointer to the array of bytes to send.
   * @see onReceive
   */
  virtual const void* onSend(std::size_t& size);

  /**
   * @brief Called after the packet is received over the network.
   *
   * This function can be defined by derived classes to
   * transform the data after it is received; this can be
   * used for decompression, decryption, etc.
   * The function receives a pointer to the received data,
   * and must fill the packet with the transformed bytes.
   * The default implementation fills the packet directly
   * without transforming the data.
   *
   * @param data Pointer to the received bytes.
   * @param size Number of bytes.
   * @see onSend
   */
  virtual void onReceive(const void* data, std::size_t size);

 private:
  /**
   * @brief Check if the packet can extract a given number of bytes.
   *
   * This function updates accordingly the state of the packet.
   *
   * @param size Size to check.
   * @return true if size bytes can be read from the packet.
   */
  bool checkSize(std::size_t size);

  std::vector<std::byte> m_data;  ///< Data stored in the packet.
  std::size_t m_readPos{};        ///< Current reading position in the packet.
  std::size_t m_sendPos{};        ///< Current send position in the packet.
  bool m_isValid{true};           ///< Reading state of the packet.
};

}  // namespace sockpp
