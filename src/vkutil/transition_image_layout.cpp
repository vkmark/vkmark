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

#include "transition_image_layout.h"

#include "managed_resource.h"
#include "vulkan_state.h"

namespace
{

vk::AccessFlags access_mask_for_layout(vk::ImageLayout layout)
{
    vk::AccessFlags ret;

    switch (layout)
    {
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            ret = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                  vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        default:
            break;
    };

    return ret;
}

}

void vkutil::transition_image_layout(
    VulkanState& vulkan,
    vk::Image image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::ImageAspectFlags aspect_mask)
{
    auto const command_buffer_allocate_info = vk::CommandBufferAllocateInfo{}
        .setCommandPool(vulkan.command_pool())
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    auto command_buffer = ManagedResource<vk::CommandBuffer>{
        std::move(vulkan.device().allocateCommandBuffers(command_buffer_allocate_info)[0]),
        [&vulkan] (auto const& cb)
        {
            vulkan.device().freeCommandBuffers(vulkan.command_pool(), cb);
        }};

    auto const image_subresource_range = vk::ImageSubresourceRange{}
        .setAspectMask(aspect_mask)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    auto const image_memory_barrier = vk::ImageMemoryBarrier{}
        .setImage(image)
        .setOldLayout(old_layout)
        .setNewLayout(new_layout)
        .setSrcAccessMask(access_mask_for_layout(old_layout))
        .setDstAccessMask(access_mask_for_layout(new_layout))
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(image_subresource_range);

    auto const begin_info = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    command_buffer.raw.begin(begin_info);

    command_buffer.raw.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTopOfPipe,
        {}, {}, {},
        image_memory_barrier);

    command_buffer.raw.end();

    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffer.raw);

    vulkan.graphics_queue().submit(submit_info, {});

    vulkan.device().waitIdle();
}
