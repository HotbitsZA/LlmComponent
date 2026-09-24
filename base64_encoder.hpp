#pragma once

#include <cstdint>
#include <string>

namespace base64_detail
{
    inline constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    inline void appendGroup(std::string &out, const char *p, std::size_t n)
    {
        const auto b0 = static_cast<std::uint8_t>(p[0]);
        const auto b1 = n > 1 ? static_cast<std::uint8_t>(p[1]) : 0;
        const auto b2 = n > 2 ? static_cast<std::uint8_t>(p[2]) : 0;

        out += kTable[b0 >> 2];
        out += kTable[((b0 & 0x03) << 4) | (b1 >> 4)];
        out += n > 1 ? kTable[((b1 & 0x0F) << 2) | (b2 >> 6)] : '=';
        out += n > 2 ? kTable[b2 & 0x3F] : '=';
    }
} // namespace base64_detail

// RFC 4648 Base64 with padding. Pure, dependency-free, testable.
inline std::string base64_encode(const std::uint8_t *data, std::size_t size)
{
    if (data == nullptr || size == 0)
    {
        return std::string();
    }

    std::string out;
    out.reserve(((size + 2) / 3) * 4);

    std::size_t i = 0;
    for (; i + 3 <= size; i += 3)
    {
        base64_detail::appendGroup(out, reinterpret_cast<const char *>(data + i), 3);
    }
    if (i < size)
    {
        base64_detail::appendGroup(out, reinterpret_cast<const char *>(data + i), size - i);
    }
    return out;
}

// Convenience overload for string payloads (e.g. raw file bytes).
inline std::string base64_encode(const std::string &data)
{
    return base64_encode(reinterpret_cast<const std::uint8_t *>(data.data()), data.size());
}