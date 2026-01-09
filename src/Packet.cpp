/**
 * @file Packet.cpp
 * @brief Network packet implementation.
 *
 * sockpp - Simple C++ Socket Library
 */

#include <sockpp/Packet.h>

#include <array>
#include <cassert>
#include <cstring>
#include <cwchar>

#include "SocketImpl.h"

namespace {

/**
 * @brief Helper function to convert bytes to integer.
 */
template <typename T, typename... Bytes>
[[nodiscard]] constexpr T ToInteger(Bytes... bytes) {
  static_assert(sizeof(T) == sizeof...(Bytes));

  T result = 0;
  std::size_t i = 0;

  // Fold expression to shift and OR each byte into result.
  ((result |= static_cast<T>(static_cast<T>(bytes) << 8 * i++)), ...);

  return result;
}

}  // namespace

namespace sockpp {

void Packet::append(const void* data, std::size_t sizeInBytes) {
  if (data && (sizeInBytes > 0)) {
    const auto* begin = reinterpret_cast<const std::byte*>(data);
    const auto* end = begin + sizeInBytes;
    m_data.insert(m_data.end(), begin, end);
  }
}

std::size_t Packet::getReadPosition() const { return m_readPos; }

void Packet::clear() {
  m_data.clear();
  m_readPos = 0;
  m_isValid = true;
}

const void* Packet::getData() const { return !m_data.empty() ? m_data.data() : nullptr; }

std::size_t Packet::getDataSize() const { return m_data.size(); }

bool Packet::endOfPacket() const { return m_readPos >= m_data.size(); }

Packet::operator bool() const { return m_isValid; }

Packet& Packet::operator>>(bool& data) {
  std::uint8_t value = 0;
  if (*this >> value) {
    data = (value != 0);
  }

  return *this;
}

Packet& Packet::operator>>(std::int8_t& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::uint8_t& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::int16_t& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    data = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(data)));
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::uint16_t& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    data = ntohs(data);
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::int32_t& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    data = static_cast<std::int32_t>(ntohl(static_cast<std::uint32_t>(data)));
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::uint32_t& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    data = ntohl(data);
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::int64_t& data) {
  if (checkSize(sizeof(data))) {
    // Since ntohll is not available everywhere, we have to convert
    // to network byte order (big endian) manually.
    std::array<std::byte, sizeof(data)> bytes{};
    std::memcpy(bytes.data(), &m_data[m_readPos], bytes.size());

    data = ToInteger<std::int64_t>(bytes[7], bytes[6], bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);

    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(std::uint64_t& data) {
  if (checkSize(sizeof(data))) {
    // Since ntohll is not available everywhere, we have to convert
    // to network byte order (big endian) manually.
    std::array<std::byte, sizeof(data)> bytes{};
    std::memcpy(bytes.data(), &m_data[m_readPos], sizeof(data));

    data = ToInteger<std::uint64_t>(bytes[7], bytes[6], bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);

    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(float& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(double& data) {
  if (checkSize(sizeof(data))) {
    std::memcpy(&data, &m_data[m_readPos], sizeof(data));
    m_readPos += sizeof(data);
  }

  return *this;
}

Packet& Packet::operator>>(char* data) {
  assert(data && "Packet::operator>> Data must not be null");

  // First extract string length.
  std::uint32_t length = 0;
  *this >> length;

  if ((length > 0) && checkSize(length)) {
    // Then extract characters.
    std::memcpy(data, &m_data[m_readPos], length);
    data[length] = '\0';

    // Update reading position.
    m_readPos += length;
  }

  return *this;
}

Packet& Packet::operator>>(std::string& data) {
  // First extract string length.
  std::uint32_t length = 0;
  *this >> length;

  data.clear();
  if ((length > 0) && checkSize(length)) {
    // Then extract characters.
    data.assign(reinterpret_cast<char*>(&m_data[m_readPos]), length);

    // Update reading position.
    m_readPos += length;
  }

  return *this;
}

Packet& Packet::operator>>(wchar_t* data) {
  assert(data && "Packet::operator>> Data must not be null");

  // First extract string length.
  std::uint32_t length = 0;
  *this >> length;

  if ((length > 0) && checkSize(length * sizeof(std::uint32_t))) {
    // Then extract characters.
    for (std::uint32_t i = 0; i < length; ++i) {
      std::uint32_t character = 0;
      *this >> character;
      data[i] = static_cast<wchar_t>(character);
    }
    data[length] = L'\0';
  }

  return *this;
}

Packet& Packet::operator>>(std::wstring& data) {
  // First extract string length.
  std::uint32_t length = 0;
  *this >> length;

  data.clear();
  if ((length > 0) && checkSize(length * sizeof(std::uint32_t))) {
    // Then extract characters.
    for (std::uint32_t i = 0; i < length; ++i) {
      std::uint32_t character = 0;
      *this >> character;
      data += static_cast<wchar_t>(character);
    }
  }

  return *this;
}

Packet& Packet::operator>>(std::u32string& data) {
  // First extract the string length.
  std::uint32_t length = 0;
  *this >> length;

  data.clear();
  if ((length > 0) && checkSize(length * sizeof(std::uint32_t))) {
    // Then extract characters.
    for (std::uint32_t i = 0; i < length; ++i) {
      std::uint32_t character = 0;
      *this >> character;
      data += static_cast<char32_t>(character);
    }
  }

  return *this;
}

Packet& Packet::operator<<(bool data) {
  *this << static_cast<std::uint8_t>(data);
  return *this;
}

Packet& Packet::operator<<(std::int8_t data) {
  append(&data, sizeof(data));
  return *this;
}

Packet& Packet::operator<<(std::uint8_t data) {
  append(&data, sizeof(data));
  return *this;
}

Packet& Packet::operator<<(std::int16_t data) {
  const auto to_write = static_cast<std::int16_t>(htons(static_cast<std::uint16_t>(data)));
  append(&to_write, sizeof(to_write));
  return *this;
}

Packet& Packet::operator<<(std::uint16_t data) {
  const std::uint16_t to_write = htons(data);
  append(&to_write, sizeof(to_write));
  return *this;
}

Packet& Packet::operator<<(std::int32_t data) {
  const auto to_write = static_cast<std::int32_t>(htonl(static_cast<std::uint32_t>(data)));
  append(&to_write, sizeof(to_write));
  return *this;
}

Packet& Packet::operator<<(std::uint32_t data) {
  const std::uint32_t to_write = htonl(data);
  append(&to_write, sizeof(to_write));
  return *this;
}

Packet& Packet::operator<<(std::int64_t data) {
  // Since htonll is not available everywhere, we have to convert
  // to network byte order (big endian) manually.

  const std::array to_write = {
      static_cast<std::uint8_t>((data >> 56) & 0xFF), static_cast<std::uint8_t>((data >> 48) & 0xFF),
      static_cast<std::uint8_t>((data >> 40) & 0xFF), static_cast<std::uint8_t>((data >> 32) & 0xFF),
      static_cast<std::uint8_t>((data >> 24) & 0xFF), static_cast<std::uint8_t>((data >> 16) & 0xFF),
      static_cast<std::uint8_t>((data >> 8) & 0xFF),  static_cast<std::uint8_t>((data) & 0xFF)};

  append(to_write.data(), to_write.size());
  return *this;
}

Packet& Packet::operator<<(std::uint64_t data) {
  // Since htonll is not available everywhere, we have to convert
  // to network byte order (big endian) manually.

  const std::array to_write = {
      static_cast<std::uint8_t>((data >> 56) & 0xFF), static_cast<std::uint8_t>((data >> 48) & 0xFF),
      static_cast<std::uint8_t>((data >> 40) & 0xFF), static_cast<std::uint8_t>((data >> 32) & 0xFF),
      static_cast<std::uint8_t>((data >> 24) & 0xFF), static_cast<std::uint8_t>((data >> 16) & 0xFF),
      static_cast<std::uint8_t>((data >> 8) & 0xFF),  static_cast<std::uint8_t>((data) & 0xFF)};

  append(to_write.data(), to_write.size());
  return *this;
}

Packet& Packet::operator<<(float data) {
  append(&data, sizeof(data));
  return *this;
}

Packet& Packet::operator<<(double data) {
  append(&data, sizeof(data));
  return *this;
}

Packet& Packet::operator<<(const char* data) {
  assert(data && "Packet::operator<< Data must not be null");

  // First insert string length.
  const auto length = static_cast<std::uint32_t>(std::strlen(data));
  *this << length;

  // Then insert characters.
  append(data, length * sizeof(char));

  return *this;
}

Packet& Packet::operator<<(const std::string& data) {
  // First insert string length.
  const auto length = static_cast<std::uint32_t>(data.size());
  *this << length;

  // Then insert characters.
  if (length > 0) {
    append(data.c_str(), length * sizeof(std::string::value_type));
  }

  return *this;
}

Packet& Packet::operator<<(const wchar_t* data) {
  assert(data && "Packet::operator<< Data must not be null");

  // First insert string length.
  const auto length = static_cast<std::uint32_t>(std::wcslen(data));
  *this << length;

  // Then insert characters.
  for (const wchar_t* c = data; *c != L'\0'; ++c) {
    *this << static_cast<std::uint32_t>(*c);
  }

  return *this;
}

Packet& Packet::operator<<(const std::wstring& data) {
  // First insert string length.
  const auto length = static_cast<std::uint32_t>(data.size());
  *this << length;

  // Then insert characters.
  if (length > 0) {
    for (const wchar_t c : data) {
      *this << static_cast<std::uint32_t>(c);
    }
  }

  return *this;
}

Packet& Packet::operator<<(const std::u32string& data) {
  // First insert the string length.
  const auto length = static_cast<std::uint32_t>(data.size());
  *this << length;

  // Then insert characters.
  if (length > 0) {
    for (const char32_t c : data) {
      *this << static_cast<std::uint32_t>(c);
    }
  }

  return *this;
}

bool Packet::checkSize(std::size_t size) {
  // Determine if size is big enough to trigger an overflow.
  const bool overflow_detected = m_readPos + size < m_readPos;
  m_isValid = m_isValid && (m_readPos + size <= m_data.size()) && !overflow_detected;

  return m_isValid;
}

const void* Packet::onSend(std::size_t& size) {
  size = getDataSize();
  return getData();
}

void Packet::onReceive(const void* data, std::size_t size) { append(data, size); }

}  // namespace sockpp
