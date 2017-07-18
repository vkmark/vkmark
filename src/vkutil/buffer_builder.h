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

#include <vulkan/vulkan.hpp>

#include "managed_resource.h"

class VulkanState;

namespace vkutil
{

class BufferBuilder
{
public:
    BufferBuilder(VulkanState& vulkan);

    BufferBuilder& set_size(size_t size);
    BufferBuilder& set_usage(vk::BufferUsageFlags usage);
    BufferBuilder& set_memory_properties(vk::MemoryPropertyFlags memory_properties);
    BufferBuilder& set_memory_out(vk::DeviceMemory& memory_out);

    ManagedResource<vk::Buffer> build();

private:
    uint32_t find_matching_memory_type_for(vk::MemoryRequirements const& requirements);

    VulkanState& vulkan;
    size_t size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memory_properties;
    vk::DeviceMemory* memory_out_ptr;
};

}
