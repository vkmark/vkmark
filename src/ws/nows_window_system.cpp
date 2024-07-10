/*
 * Copyright © 2024 Igalia S.L.
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
 *   Lucas Fryzek <lfryzek@igalia.com>
 */

#include "nows_window_system.h"

#include "vulkan_state.h"
#include "vulkan_image.h"
#include "vkutil/vkutil.h"
#include "log.h"

#include <stdexcept>
#include <algorithm>
#include <cctype>

NoWindowSystem::NoWindowSystem(
    vk::Format pixel_format, uint32_t width, uint32_t height, uint32_t num_buffers)
    : vk_pixel_format{pixel_format},
      vulkan{nullptr},
      vk_extent{width, height},
      num_buffers{num_buffers}
{
}

VulkanWSI& NoWindowSystem::vulkan_wsi()
{
    return *this;
}

void NoWindowSystem::init_vulkan(VulkanState& vulkan_)
{
    vulkan = &vulkan_;

    vk_queue_family_index = vulkan->graphics_queue_family_index();
    vk_queue = vulkan->device().getQueue(vk_queue_family_index, 0);

    Log::debug("NoWindowSystem: Allocating %u %ux%u buffers\n",
            num_buffers, vk_extent.width, vk_extent.height);

    for (uint32_t i = 0; i < num_buffers; ++i)
    {
        vk_images.push_back(
            vkutil::ImageBuilder(*vulkan)
                .set_extent(vk_extent)
                .set_format(vk_pixel_format)
                .set_tiling(vk::ImageTiling::eOptimal)
                .set_usage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
                .set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal)
                .set_initial_layout(vk::ImageLayout::eUndefined)
                .build()
        );

        vkutil::transition_image_layout(
                *vulkan,
                vk_images[i],
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageAspectFlagBits::eColor);

        vk_acquire_fences.push_back(ManagedResource<vk::Fence>{
            vulkan->device().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
            [this] (auto& f) {vulkan->device().destroyFence(f); }});
    }

    vk_semaphore = nullptr;
    current_frame = 0;
}

void NoWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();
    vk_semaphore = nullptr;
    vk_images.clear();
    vk_acquire_fences.clear();
}

VulkanImage NoWindowSystem::next_vulkan_image()
{
    vulkan->device().waitForFences(vk_acquire_fences[current_frame].raw, true, INT64_MAX);
    vulkan->device().resetFences(vk_acquire_fences[current_frame].raw);
    return {current_frame, vk_images[current_frame], vk_pixel_format, vk_extent, vk_semaphore, vk_acquire_fences[current_frame]};
}

void NoWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    vk_semaphore = vulkan_image.semaphore;
    current_frame = (current_frame + 1) % vk_images.size();
}

std::vector<VulkanImage> NoWindowSystem::vulkan_images()
{
    std::vector<VulkanImage> vulkan_images;

    for (uint32_t i = 0; i < vk_images.size(); ++i)
    {
        vulkan_images.push_back({i, vk_images[i], vk_pixel_format, vk_extent, {}, {}});
    }

    vk_semaphore = nullptr;
    return vulkan_images;
}

bool NoWindowSystem::should_quit()
{
    return false;
}

VulkanWSI::Extensions NoWindowSystem::required_extensions()
{
    return {{}, {VK_KHR_SWAPCHAIN_EXTENSION_NAME}};
}

bool NoWindowSystem::is_physical_device_supported(vk::PhysicalDevice const& pd)
{
    return true;
}

std::vector<uint32_t> NoWindowSystem::physical_device_queue_family_indices(
    vk::PhysicalDevice const& pd)
{
    return {};
}
