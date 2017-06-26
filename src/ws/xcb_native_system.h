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

#define VK_USE_PLATFORM_XCB_KHR
#include "native_system.h"

#include <xcb/xcb.h>

struct Options;

class XcbNativeSystem : public NativeSystem
{
public:
    XcbNativeSystem(int width, int height);
    ~XcbNativeSystem();

    std::vector<char const*> vulkan_extensions() override;
    bool should_quit() override;
    vk::Extent2D get_vk_extent() override;
    ManagedResource<vk::SurfaceKHR> create_vk_surface(VulkanState& vulkan) override;

private:
    void create_native_window();
    xcb_atom_t atom(std::string const& name);
    bool fullscreen_requested();

    int const requested_width;
    int const requested_height;

    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_visualid_t root_visual;
    xcb_atom_t atom_wm_protocols;
    xcb_atom_t atom_wm_delete_window;
    vk::Extent2D vk_extent;
};
