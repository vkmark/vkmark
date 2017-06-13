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

#include "window_system.h"

#define VK_USE_PLATFORM_WAYLAND_KHR
#include "vulkan/vulkan.hpp"

#include "managed_resource.h"

#include <memory>
#include <wayland-client.h>

struct Options;

class WaylandWindowSystem : public WindowSystem
{
public:
    WaylandWindowSystem(int width, int height, vk::PresentModeKHR present_mode);

    std::vector<char const*> vulkan_extensions() override;
    void init_vulkan(VulkanState& vulkan) override;
    void deinit_vulkan() override;

    VulkanImage next_vulkan_image() override;
    void present_vulkan_image(VulkanImage const&) override;

    bool should_quit() override;

private:
    void create_native_window();
    void create_swapchain();

    static void handle_registry_global(
        void* data, wl_registry* registry, uint32_t id,
        char const* interface, uint32_t version);
    static void handle_seat_capabilities(
        void* data, wl_seat* seat, uint32_t capabilities);
    static void handle_keyboard_key(
        void* data, wl_keyboard* wl_keyboard,
        uint32_t serial, uint32_t time,
        uint32_t key, uint32_t state);

    static wl_seat_listener const seat_listener;
    static wl_keyboard_listener const keyboard_listener;

    int const width;
    int const height;
    vk::PresentModeKHR const vk_present_mode;
    bool should_quit_;

    ManagedResource<wl_display*> display;
    ManagedResource<wl_compositor*> compositor;
    ManagedResource<wl_shell*> shell;
    ManagedResource<wl_seat*> seat;
    ManagedResource<wl_keyboard*> keyboard;
    ManagedResource<wl_surface*> surface;
    ManagedResource<wl_shell_surface*> shell_surface;
    int display_fd;

    VulkanState* vulkan;
    ManagedResource<vk::SurfaceKHR> vk_surface;
    ManagedResource<vk::SwapchainKHR> vk_swapchain;
    ManagedResource<vk::Semaphore> vk_acquire_semaphore;
    std::vector<vk::Image> vk_images;
    vk::Format vk_image_format;
};

extern "C" int vkmark_window_system_probe();
extern "C" std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options);
