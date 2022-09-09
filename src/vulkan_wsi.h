/*
 * Copyright © 2017 Collabora Ltd.
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
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

#pragma once

#include <vector>
#include <cstdint>

namespace vk { class PhysicalDevice; }

class VulkanWSI
{
public:
    virtual ~VulkanWSI() = default;

    struct Extensions
    {
        std::vector<char const*> instance;
        std::vector<char const*> device;
    };

    virtual Extensions required_extensions() = 0;
    virtual bool is_physical_device_supported(vk::PhysicalDevice const& pd) = 0;
    virtual std::vector<uint32_t> physical_device_queue_family_indices(
        vk::PhysicalDevice const& pd) = 0;

protected:
    VulkanWSI() = default;
    VulkanWSI(VulkanWSI const&) = delete;
    VulkanWSI& operator=(VulkanWSI const&) = delete;
};
