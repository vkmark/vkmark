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

# pragma once

#include "window_system.h"
#include "vulkan_wsi.h"
#include "managed_resource.h"

#include <memory.h>
#include <vulkan/vulkan.hpp>

class NativeSystem;

class NoWindowSystem : public WindowSystem, VulkanWSI
{
public:
    NoWindowSystem(
            vk::Format pixel_format, uint32_t width, uint32_t height, uint32_t num_buffers);

    VulkanWSI& vulkan_wsi() override;
    void init_vulkan(VulkanState& vulkan) override;
    void deinit_vulkan() override;

    VulkanImage next_vulkan_image() override;
    void present_vulkan_image(VulkanImage const&) override;
    std::vector<VulkanImage> vulkan_images() override;

    bool should_quit() override;

    // VulkanWSI
    Extensions required_extensions() override;
    bool is_physical_device_supported(vk::PhysicalDevice const& pd) override;
    std::vector<uint32_t> physical_device_queue_family_indices(
        vk::PhysicalDevice const& pd) override;
private:
    vk::Format const vk_pixel_format;
    VulkanState* vulkan;
    uint32_t vk_queue_family_index;
    vk::Queue vk_queue;
    uint32_t current_frame;
    vk::Semaphore vk_semaphore;
    std::vector<ManagedResource<vk::Fence>> vk_acquire_fences;
    std::vector<ManagedResource<vk::Image>> vk_images;
    vk::Extent2D vk_extent;
    uint32_t num_buffers;
};
