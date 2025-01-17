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
#include "log.h"

#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace
{

bool is_format_srgb(vk::Format f)
{
    return to_string(f).find("Srgb") != std::string::npos;
}

int format_bits(vk::Format f)
{
    int total = 0;
    int current = 0;
    bool process_digits = false;

    for (auto const c : to_string(f))
    {
        if (process_digits && std::isdigit(static_cast<unsigned char>(c)))
        {
            current = current * 10 + c - '0';
        }
        else
        {
            process_digits = (c == 'R' || c == 'G' || c == 'B' || c == 'A');
            total += current;
            current = 0;
        }
    }

    total += current;

    return total;
}

struct SurfaceFormatInfo
{
    SurfaceFormatInfo(vk::SurfaceFormatKHR f)
        : format{f},
          srgb{is_format_srgb(f.format)},
          bits{format_bits(f.format)}
    {
    }

    vk::SurfaceFormatKHR format;
    bool srgb;
    int bits;
};

vk::SurfaceFormatKHR select_surface_format(
    std::vector<vk::SurfaceFormatKHR> const& formats)
{
    if (formats.empty())
        return {};

    std::vector<SurfaceFormatInfo> format_infos;

    for (auto const& f : formats)
        format_infos.emplace_back(f);

    std::sort(
        format_infos.begin(), format_infos.end(),
        [] (auto const& a, auto const& b)
        {
            return (a.srgb && !b.srgb) || a.bits > b.bits;
        });

    return format_infos[0].format;
}

}

SwapchainWindowSystem::SwapchainWindowSystem(
    std::unique_ptr<NativeSystem> native,
    vk::PresentModeKHR present_mode,
    vk::Format pixel_format)
    : native{std::move(native)},
      vk_present_mode{present_mode},
      vk_pixel_format{pixel_format},
      vulkan{nullptr}
{
}

VulkanWSI& SwapchainWindowSystem::vulkan_wsi()
{
    return *this;
}

void SwapchainWindowSystem::init_vulkan(VulkanState& vulkan_)
{
    vulkan = &vulkan_;

    vk_present_queue_family_index =
        native->get_presentation_queue_family_index(vulkan->physical_device());
    if (vk_present_queue_family_index == NativeSystem::invalid_queue_family_index)
    {
        throw std::runtime_error{
            "Physical device doesn't have a queue family that supports "
            "presentation on the selected window system"};
    }
    vk_present_queue = vulkan->device().getQueue(vk_present_queue_family_index, 0);

    vk_surface = native->create_vk_surface(vulkan_);
    // Get the extent after creating the surface, since some window system plugins
    // only know the extent after that point.
    vk_extent = native->get_vk_extent();
    vk_swapchain = create_vk_swapchain();
    vk_images = vulkan->device().getSwapchainImagesKHR(vk_swapchain);

    Log::debug("SwapchainWindowSystem: Swapchain contains %d images\n",
               vk_images.size());

    for (uint32_t i = 0; i < vk_images.size(); i++)
    {
        vk_acquire_semaphores.push_back(ManagedResource<vk::Semaphore>{
            vulkan->device().createSemaphore(vk::SemaphoreCreateInfo()),
            [this] (auto& s) { vulkan->device().destroySemaphore(s); }});
        vk_acquire_fences.push_back(ManagedResource<vk::Fence>{
            vulkan->device().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
            [this] (auto& f) { vulkan->device().destroyFence(f); }});
    }

    current_frame = 0;
}

void SwapchainWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();
    vk_acquire_semaphores.clear();
    vk_acquire_fences.clear();
    vk_swapchain = {};
    vk_surface = {};
}

VulkanImage SwapchainWindowSystem::next_vulkan_image()
{
    (void)vulkan->device().waitForFences(vk_acquire_fences[current_frame].raw, true, INT64_MAX);
    vulkan->device().resetFences(vk_acquire_fences[current_frame].raw);

    auto const image_index = vulkan->device().acquireNextImageKHR(
        vk_swapchain, UINT64_MAX, vk_acquire_semaphores[current_frame], vk_acquire_fences[current_frame]).value;

    return {image_index, vk_images[image_index], vk_image_format, vk_extent, vk_acquire_semaphores[current_frame]};
}

void SwapchainWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    auto const present_info = vk::PresentInfoKHR{}
        .setSwapchainCount(1)
        .setPSwapchains(&vk_swapchain.raw)
        .setPImageIndices(&vulkan_image.index)
        .setWaitSemaphoreCount(vulkan_image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&vulkan_image.semaphore);

    (void)vk_present_queue.presentKHR(present_info);

    current_frame = (current_frame + 1) % vk_acquire_semaphores.size();
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
            vk_present_queue_family_index,
            vk_surface))
    {
        throw std::runtime_error("Surface not supported");
    }

    auto const surface_formats = vulkan->physical_device().getSurfaceFormatsKHR(vk_surface);

    if (vk_pixel_format != vk::Format::eUndefined)
        vk_image_format = vk_pixel_format;
    else
        vk_image_format = select_surface_format(surface_formats).format;

    for (auto const& format : surface_formats)
    {
        Log::debug("SwapchainWindowSystem: Available surface format %s\n",
                   to_string(format.format).c_str());
    }

    Log::debug("SwapchainWindowSystem: Selected swapchain format %s\n",
               to_string(vk_image_format).c_str());

    auto const present_modes = vulkan->physical_device().getSurfacePresentModesKHR(vk_surface);
    if (std::find(present_modes.begin(), present_modes.end(), vk_present_mode) ==
            present_modes.end())
    {
        throw std::runtime_error{
            "Selected present mode " + to_string(vk_present_mode) +
            " is not supported by the used Vulkan physical device."};
    }

    // Try to enable triple buffering
    auto min_image_count = std::max(surface_caps.minImageCount, 3u);
    if (surface_caps.maxImageCount > 0)
        min_image_count = std::min(min_image_count, surface_caps.maxImageCount);

    auto const swapchain_create_info = vk::SwapchainCreateInfoKHR{}
        .setSurface(vk_surface)
        .setMinImageCount(min_image_count)
        .setImageFormat(vk_image_format)
        .setImageExtent(vk_extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                       vk::ImageUsageFlagBits::eTransferDst)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setQueueFamilyIndexCount(1)
        .setPQueueFamilyIndices(&vk_present_queue_family_index)
        .setPresentMode(vk_present_mode);

    return ManagedResource<vk::SwapchainKHR>{
        vulkan->device().createSwapchainKHR(swapchain_create_info),
        [this] (auto& s) { vulkan->device().destroySwapchainKHR(s); }};
}

VulkanWSI::Extensions SwapchainWindowSystem::required_extensions()
{
    auto extensions = native->required_extensions();
    extensions.device.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    return extensions;
}

bool SwapchainWindowSystem::is_physical_device_supported(vk::PhysicalDevice const& pd)
{
    return native->get_presentation_queue_family_index(pd) !=
               NativeSystem::invalid_queue_family_index;
}

std::vector<uint32_t> SwapchainWindowSystem::physical_device_queue_family_indices(
    vk::PhysicalDevice const& pd)
{
    return {native->get_presentation_queue_family_index(pd)};
}
