/*
 * Copyright © 2018 Collabora Ltd.
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

#include "map_memory.h"

#include "vulkan_state.h"

ManagedResource<void*> vkutil::map_memory(
    VulkanState& vulkan,
    vk::DeviceMemory memory,
    vk::DeviceSize offset,
    vk::DeviceSize size,
    vk::MemoryMapFlags flags)
{
    return ManagedResource<void*>{
        vulkan.device().mapMemory(memory, offset, size, flags),
        [vptr=&vulkan, memory] (auto const&) { vptr->device().unmapMemory(memory); }};
}
