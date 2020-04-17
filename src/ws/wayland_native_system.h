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

#define VK_USE_PLATFORM_WAYLAND_KHR
#include "native_system.h"

#include <wayland-client.h>

struct Options;

class WaylandNativeSystem : public NativeSystem
{
public:
    WaylandNativeSystem(int width, int height);

    std::vector<char const*> vulkan_extensions() override;
    uint32_t get_presentation_queue_family_index(vk::PhysicalDevice const& pd) override;
    bool should_quit() override;
    vk::Extent2D get_vk_extent() override;
    ManagedResource<vk::SurfaceKHR> create_vk_surface(VulkanState& vulkan) override;

private:
    void create_native_window();
    bool fullscreen_requested();

    static void handle_registry_global(
        void* data, wl_registry* registry, uint32_t id,
        char const* interface, uint32_t version);
    static void handle_seat_capabilities(
        void* data, wl_seat* seat, uint32_t capabilities);
    static void handle_output_mode(
        void* data, wl_output* output,
        uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    static void handle_output_scale(
        void* data, wl_output* output, int32_t factor);
    static void handle_keyboard_key(
        void* data, wl_keyboard* wl_keyboard,
        uint32_t serial, uint32_t time,
        uint32_t key, uint32_t state);

    static wl_seat_listener const seat_listener;
    static wl_keyboard_listener const keyboard_listener;
    static wl_output_listener const output_listener;

    int const requested_width;
    int const requested_height;
    bool should_quit_;

    ManagedResource<wl_display*> display;
    ManagedResource<wl_compositor*> compositor;
    ManagedResource<wl_shell*> shell;
    ManagedResource<wl_seat*> seat;
    ManagedResource<wl_output*> output;
    ManagedResource<wl_keyboard*> keyboard;
    ManagedResource<wl_surface*> surface;
    ManagedResource<wl_shell_surface*> shell_surface;
    int display_fd;
    int32_t output_width;
    int32_t output_height;
    int32_t output_refresh;
    int32_t output_scale;
    vk::Extent2D vk_extent;
};
