#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vulkan/vulkan.hpp>


struct DeviceUUID
{
    std::array<uint8_t, VK_UUID_SIZE> raw{};
    
    DeviceUUID() = default;
    DeviceUUID(std::array<uint8_t, VK_UUID_SIZE> const& bytes)
        : raw(bytes)
    {}

    DeviceUUID(std::string const& representation);

    DeviceUUID(const uint8_t bytes[]) // TODO: only used for githubs CI. delete when not needed anymore
    {
        std::copy(bytes, bytes + VK_UUID_SIZE, raw.data());
    }

    operator std::array<uint8_t, VK_UUID_SIZE> () const 
    {
        return raw;
    }

    bool operator==(const DeviceUUID& other) const
    {
        return raw == other.raw;
    } 

    std::array<char, 2 * VK_UUID_SIZE + 1> representation() const;
};