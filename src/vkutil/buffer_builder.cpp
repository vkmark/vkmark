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

#include "buffer_builder.h"

#include "vulkan_state.h"

vkutil::BufferBuilder::BufferBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      size{0},
      memory_out_ptr{nullptr}
{
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_size(size_t size_)
{
    size = size_;
    return *this;
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_usage(vk::BufferUsageFlags usage_)
{
    usage = usage_;
    return *this;
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_memory_properties(
    vk::MemoryPropertyFlags memory_properties_)
{
    memory_properties = memory_properties_;
    return *this;
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_memory_out(
    vk::DeviceMemory& memory_out)
{
    memory_out_ptr = &memory_out;
    return *this;
}

ManagedResource<vk::Buffer> vkutil::BufferBuilder::build()
{
    auto const vertex_buffer_create_info = vk::BufferCreateInfo{}
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    auto vk_buffer = ManagedResource<vk::Buffer>{
        vulkan.device().createBuffer(vertex_buffer_create_info),
        [vptr=&vulkan] (auto const& b) { vptr->device().destroyBuffer(b); }};

    auto const mem_requirements = vulkan.device().getBufferMemoryRequirements(vk_buffer);
    auto const mem_type = find_matching_memory_type_for(mem_requirements);

    auto const memory_allocate_info = vk::MemoryAllocateInfo{}
        .setAllocationSize(mem_requirements.size)
        .setMemoryTypeIndex(mem_type);

    auto vk_mem = ManagedResource<vk::DeviceMemory>{
        vulkan.device().allocateMemory(memory_allocate_info),
        [vptr=&vulkan] (auto const& m) { vptr->device().freeMemory(m); }};

    vulkan.device().bindBufferMemory(vk_buffer, vk_mem, 0);

    if (memory_out_ptr)
        *memory_out_ptr = vk_mem.raw;

    return ManagedResource<vk::Buffer>{
        vk_buffer.steal(),
        [vptr=&vulkan, mem=vk_mem.steal()]
        (auto const& b)
        {
            vptr->device().freeMemory(mem);
            vptr->device().destroyBuffer(b);
        }};
}

uint32_t vkutil::BufferBuilder::find_matching_memory_type_for(
    vk::MemoryRequirements const& requirements)
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

    throw std::runtime_error("Couldn't find mathcing memory type for buffer");
}
