#pragma once

#include <array>
#include <vulkan/vulkan.hpp>

using DeviceUUID = std::array<unsigned char, VK_UUID_SIZE>;

template<std::size_t Size>
constexpr std::array<char, 2 * Size + 1> decode_UUID(const std::array<unsigned char, Size>& bytes)
{
    std::array<char, 2 * Size + 1> representation;
    constexpr char characters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        auto& byte = bytes[i];

        representation[2 * i] = characters[byte / 16];
        representation[2 * i + 1] = characters[byte % 16];
    }
    representation.back() = '\0';

    return representation;
}

template<std::size_t Size>
constexpr std::array<unsigned char, Size> encode_UUID(const std::array<char, 2 * Size + 1>& representation)
{
    std::array<unsigned char, Size> bytes;

    auto&& from_character = [](const char ch){
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        throw std::invalid_argument(std::string{ch} + "character found while parsing hexadecimal string!");
    };

    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        bytes[i] = from_character(representation[2 * i]) * 16 + from_character(representation[2 * i + 1]);
    }

    return bytes;
}