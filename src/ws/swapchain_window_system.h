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

#include "window_system.h"
#include "vulkan_wsi.h"
#include "managed_resource.h"

#include <vulkan/vulkan.hpp>

class NativeSystem;

class SwapchainWindowSystem : public WindowSystem, public VulkanWSI
{
public:
    SwapchainWindowSystem(
        std::unique_ptr<NativeSystem> native,
        vk::PresentModeKHR present_mode,
        vk::Format pixel_format);

    VulkanWSI& vulkan_wsi() override;
    void init_vulkan(VulkanState& vulkan) override;
    void deinit_vulkan() override;

    VulkanImage next_vulkan_image() override;
    void present_vulkan_image(VulkanImage const&) override;
    std::vector<VulkanImage> vulkan_images() override;

    bool should_quit() override;

    // VulkanWSI
    std::vector<char const*> vulkan_extensions() override;
    bool is_physical_device_supported(vk::PhysicalDevice const& pd) override;
    std::vector<uint32_t> physical_device_queue_family_indices(
        vk::PhysicalDevice const& pd) override;

private:
    ManagedResource<vk::SwapchainKHR> create_vk_swapchain();

    std::unique_ptr<NativeSystem> const native;
    vk::PresentModeKHR const vk_present_mode;
    vk::Format const vk_pixel_format;

    VulkanState* vulkan;
    uint32_t vk_present_queue_family_index;
    vk::Queue vk_present_queue;
    ManagedResource<vk::SurfaceKHR> vk_surface;
    ManagedResource<vk::SwapchainKHR> vk_swapchain;
    ManagedResource<vk::Semaphore> vk_acquire_semaphore;
    std::vector<vk::Image> vk_images;
    vk::Format vk_image_format;
    vk::Extent2D vk_extent;
};
