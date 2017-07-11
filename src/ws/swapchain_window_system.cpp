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

    if (vk_pixel_format != vk::Format::eUndefined)
        vk_image_format = vk_pixel_format;
    else
        vk_image_format = select_surface_format(surface_formats).format;

    for (auto const& format : surface_formats)
    {
        Log::debug("SwapchainWindowSystem: Avalaible surface format %s\n",
                   to_string(format.format).c_str());
    }

    Log::debug("SwapchainWindowSystem: Selected swapchain format %s\n",
               to_string(vk_image_format).c_str());

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
