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

#define VK_USE_PLATFORM_XCB_KHR
#include "vulkan/vulkan.hpp"

#include "managed_resource.h"

#include <memory>
#include <xcb/xcb.h>

struct Options;

class XcbWindowSystem : public WindowSystem
{
public:
    XcbWindowSystem(int width, int height, vk::PresentModeKHR present_mode);
    ~XcbWindowSystem();

    std::vector<char const*> vulkan_extensions() override;
    void init_vulkan(VulkanState& vulkan) override;
    void deinit_vulkan() override;

    VulkanImage next_vulkan_image() override;
    void present_vulkan_image(VulkanImage const&) override;
    std::vector<VulkanImage> vulkan_images() override;

    bool should_quit() override;

private:
    void create_native_window();
    void create_swapchain();
    xcb_atom_t atom(std::string const& name);
    bool fullscreen_requested();

    int const requested_width;
    int const requested_height;
    vk::PresentModeKHR const vk_present_mode;

    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_visualid_t root_visual;
    xcb_atom_t atom_wm_protocols;
    xcb_atom_t atom_wm_delete_window;

    VulkanState* vulkan;
    ManagedResource<vk::SurfaceKHR> vk_surface;
    ManagedResource<vk::SwapchainKHR> vk_swapchain;
    ManagedResource<vk::Semaphore> vk_acquire_semaphore;
    std::vector<vk::Image> vk_images;
    vk::Format vk_image_format;
    vk::Extent2D vk_extent;
};

extern "C" int vkmark_window_system_probe();
extern "C" std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options);
