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

#include "copy_buffer.h"

#include "managed_resource.h"
#include "vulkan_state.h"

void vkutil::copy_buffer(
    VulkanState& vulkan,
    vk::Buffer src,
    vk::Buffer dst,
    vk::DeviceSize size)
{
    auto const command_buffer_allocate_info = vk::CommandBufferAllocateInfo{}
        .setCommandPool(vulkan.command_pool())
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);


    auto copy_command_buffer = ManagedResource<vk::CommandBuffer>{
        std::move(vulkan.device().allocateCommandBuffers(command_buffer_allocate_info)[0]),
        [&vulkan] (auto const& cb)
        {
            vulkan.device().freeCommandBuffers(vulkan.command_pool(), cb);
        }};

    auto const begin_info = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    copy_command_buffer.raw.begin(begin_info);

    auto const region = vk::BufferCopy{}.setSize(size);
    copy_command_buffer.raw.copyBuffer(src, dst, region);

    copy_command_buffer.raw.end();

    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&copy_command_buffer.raw);

    vulkan.graphics_queue().submit(submit_info, {});

    vulkan.device().waitIdle();
}
