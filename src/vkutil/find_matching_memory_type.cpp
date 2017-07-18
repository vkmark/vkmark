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

#include "find_matching_memory_type.h"

#include "vulkan_state.h"

uint32_t vkutil::find_matching_memory_type(
    VulkanState& vulkan,
    vk::MemoryRequirements const& requirements,
    vk::MemoryPropertyFlags const& memory_properties)
{
    auto const properties = vulkan.physical_device().getMemoryProperties();

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Couldn't find matching memory type");
}
