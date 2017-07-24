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

#include "one_time_command_buffer.h"

#include "vulkan_state.h"

namespace
{

ManagedResource<vk::CommandBuffer> create_command_buffer(VulkanState& vulkan)
{
    auto const command_buffer_allocate_info = vk::CommandBufferAllocateInfo{}
        .setCommandPool(vulkan.command_pool())
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    return ManagedResource<vk::CommandBuffer>{
        std::move(vulkan.device().allocateCommandBuffers(command_buffer_allocate_info)[0]),
        [&vulkan] (auto const& cb)
        {
            vulkan.device().freeCommandBuffers(vulkan.command_pool(), cb);
        }};
}

}

vkutil::OneTimeCommandBuffer::OneTimeCommandBuffer(
    VulkanState& vulkan)
    : vulkan{vulkan},
      command_buffer_{create_command_buffer(vulkan)}
{
    auto const begin_info = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    command_buffer_.raw.begin(begin_info);
}

vk::CommandBuffer vkutil::OneTimeCommandBuffer::command_buffer() const
{
    return command_buffer_;
}

void vkutil::OneTimeCommandBuffer::submit()
{
    command_buffer_.raw.end();

    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffer_.raw);

    vulkan.graphics_queue().submit(submit_info, {});

    vulkan.device().waitIdle();
}

