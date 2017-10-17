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
#include "one_time_command_buffer.h"

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

vk::PipelineStageFlags pipeline_stage_flags_for_layout(vk::ImageLayout layout)
{
    vk::PipelineStageFlags ret;

    switch (layout)
    {
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            ret = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            ret = vk::PipelineStageFlagBits::eTransfer;
            break;
        default:
            ret = vk::PipelineStageFlagBits::eTopOfPipe;
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

    OneTimeCommandBuffer otcb{vulkan};

    otcb.command_buffer().pipelineBarrier(
        pipeline_stage_flags_for_layout(old_layout),
        pipeline_stage_flags_for_layout(new_layout),
        {}, {}, {},
        image_memory_barrier);

    otcb.submit();
}
