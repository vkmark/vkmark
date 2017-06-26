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

#include "swapchain_window_system.h"
#include "native_system.h"

#include "vulkan_state.h"
#include "vulkan_image.h"

#include <stdexcept>

SwapchainWindowSystem::SwapchainWindowSystem(
    std::unique_ptr<NativeSystem> native,
    vk::PresentModeKHR present_mode)
    : native{std::move(native)},
      vk_present_mode{present_mode},
      vulkan{nullptr}
{
}

std::vector<char const*> SwapchainWindowSystem::vulkan_extensions()
{
    return native->vulkan_extensions();
}

void SwapchainWindowSystem::init_vulkan(VulkanState& vulkan_)
{
    vulkan = &vulkan_;

    vk_extent = native->get_vk_extent();
    vk_surface = native->create_vk_surface(vulkan_);
    vk_swapchain = create_vk_swapchain();
    vk_images = vulkan->device().getSwapchainImagesKHR(vk_swapchain);

    vk_acquire_semaphore = ManagedResource<vk::Semaphore>{
        vulkan->device().createSemaphore(vk::SemaphoreCreateInfo()),
        [this] (auto& s) { vulkan->device().destroySemaphore(s); }};

}

void SwapchainWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();
    vk_acquire_semaphore = {};
    vk_swapchain = {};
    vk_surface = {};
}

VulkanImage SwapchainWindowSystem::next_vulkan_image()
{
    auto const image_index = vulkan->device().acquireNextImageKHR(
        vk_swapchain, UINT64_MAX, vk_acquire_semaphore, nullptr).value;

    return {image_index, vk_images[image_index], vk_image_format, vk_extent, vk_acquire_semaphore};
}

void SwapchainWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    auto const present_info = vk::PresentInfoKHR{}
        .setSwapchainCount(1)
        .setPSwapchains(&vk_swapchain.raw)
        .setPImageIndices(&vulkan_image.index)
        .setWaitSemaphoreCount(vulkan_image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&vulkan_image.semaphore);

    vulkan->graphics_queue().presentKHR(present_info);
}

std::vector<VulkanImage> SwapchainWindowSystem::vulkan_images()
{
    std::vector<VulkanImage> vulkan_images;

    for (uint32_t i = 0; i < vk_images.size(); ++i)
        vulkan_images.push_back({i, vk_images[i], vk_image_format, vk_extent, {}});

    return vulkan_images;
}

bool SwapchainWindowSystem::should_quit()
{
    return native->should_quit();
}

ManagedResource<vk::SwapchainKHR> SwapchainWindowSystem::create_vk_swapchain()
{
    auto const surface_caps = vulkan->physical_device().getSurfaceCapabilitiesKHR(vk_surface);
    if (!(surface_caps.supportedCompositeAlpha &
          vk::CompositeAlphaFlagBitsKHR::eOpaque))
    {
        throw std::runtime_error("Opaque not supported");
    }

    if (!vulkan->physical_device().getSurfaceSupportKHR(
            vulkan->graphics_queue_family_index(),
            vk_surface))
    {
        throw std::runtime_error("Surface not supported");
    }

    auto const surface_formats = vulkan->physical_device().getSurfaceFormatsKHR(vk_surface);
    for (auto const& format : surface_formats)
    {
        if (format.format == vk::Format::eR8G8B8A8Srgb ||
            format.format == vk::Format::eB8G8R8A8Srgb)
        {
            vk_image_format = format.format;
        }
    }

    auto const swapchain_create_info = vk::SwapchainCreateInfoKHR{}
        .setSurface(vk_surface)
        .setMinImageCount(2)
        .setImageFormat(vk_image_format)
        .setImageExtent(vk_extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setQueueFamilyIndexCount(1)
        .setPQueueFamilyIndices(&vulkan->graphics_queue_family_index())
        .setPresentMode(vk_present_mode);

    return ManagedResource<vk::SwapchainKHR>{
        vulkan->device().createSwapchainKHR(swapchain_create_info),
        [this] (auto& s) { vulkan->device().destroySwapchainKHR(s); }};
}
