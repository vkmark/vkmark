#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>


template<std::size_t Size>
constexpr std::array<char, 2 * Size> decode_UUID(const std::array<unsigned char, Size>& bytes)
{
    std::array<char, 2 * Size> representation;
    constexpr char characters[16] = 
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        auto& byte = bytes[i];

        representation[2 * i] = characters[byte / 16];
        representation[2 * i + 1] = characters[byte % 16];
    }

    return representation;
}

template<std::size_t Size>
constexpr std::array<unsigned char, Size> encode_UUID(const std::array<char, 2 * Size>& representation)
{
    std::array<unsigned char, Size> bytes;

    auto&& decode_character = [](const char ch){
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        throw std::invalid_argument(
            std::string{ch} + "character found while parsing hexadecimal string!");
    };

    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        bytes[i] = decode_character(representation[2 * i]) * 16
            + decode_character(representation[2 * i + 1]);
    }

    return bytes;
}

struct DeviceUUID
{
    std::array<unsigned char, VK_UUID_SIZE> raw;
    
    DeviceUUID() = default;
    DeviceUUID(std::array<unsigned char, VK_UUID_SIZE> const& bytes)
        : raw(bytes)
    {}
    DeviceUUID(std::string const& representation)
    {
        std::array<char, 2 * VK_UUID_SIZE> chars;
        if (representation.size() != chars.size())
            throw std::invalid_argument("given UUID representation has wrong size!");

        std::copy(representation.begin(), representation.end(), chars.begin());
        raw = encode_UUID<chars.size() / 2>(chars);
    }

    operator std::array<unsigned char, VK_UUID_SIZE> () const 
    {
        return raw;
    }

    operator std::string () const
    {
        auto&& chars = decode_UUID(raw);
        return std::string(chars.begin(), chars.end());
    }

    bool operator==(const DeviceUUID& other)
    {
        return raw == other.raw;
    } 

};