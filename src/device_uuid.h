/*
 * Copyright © 2021 vkmark developers
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with vkmark. If not, see <http://www.gnu.org/licenses/>.
 */

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
