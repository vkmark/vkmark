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

#include "wayland_window_system.h"
#include "options.h"

#include "vulkan_state.h"
#include "vulkan_image.h"

#include <stdexcept>
#include <linux/input.h>
#include <poll.h>

namespace
{

void handle_registry_global_remove(
    void* /*data*/, struct wl_registry* /*registry*/, uint32_t /*name*/)
{
}

void handle_keyboard_keymap(
    void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*format*/,
    int32_t /*fd*/, uint32_t /*size*/)
{
}

void handle_keyboard_enter(
    void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/,
    wl_surface* /*surface*/, wl_array* /*keys*/)
{
}

void handle_keyboard_leave(
    void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/,
    wl_surface* /*surface*/)
{
}

void handle_keyboard_modifiers(
    void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/,
    uint32_t /*mods_depressed*/, uint32_t /*mods_latched*/,
    uint32_t /*mods_locked*/, uint32_t /*group*/)
{
}

void handle_keyboard_repeat_info(
    void* /*data*/, wl_keyboard* /*wl_keyboard*/,
    int32_t /*rate*/, int32_t /*delay*/)
{
}

}

wl_seat_listener const WaylandWindowSystem::seat_listener{
    WaylandWindowSystem::handle_seat_capabilities
};

wl_keyboard_listener const WaylandWindowSystem::keyboard_listener{
    handle_keyboard_keymap,
    handle_keyboard_enter,
    handle_keyboard_leave,
    WaylandWindowSystem::handle_keyboard_key,
    handle_keyboard_modifiers,
    handle_keyboard_repeat_info
};

WaylandWindowSystem::WaylandWindowSystem(
    int width, int height, vk::PresentModeKHR present_mode)
    : width{width},
      height{height},
      vk_present_mode{present_mode},
      should_quit_{false},
      vulkan{nullptr}
{
    create_native_window();
}

std::vector<char const*> WaylandWindowSystem::vulkan_extensions()
{
    return {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
}

void WaylandWindowSystem::init_vulkan(VulkanState& vulkan_)
{
    vulkan = &vulkan_;

    auto const vk_supported = vulkan->physical_device().getWaylandPresentationSupportKHR(
        vulkan->graphics_queue_family_index(),
        display);

    if (!vk_supported)
        throw std::runtime_error{"Queue family does not support presentation on Wayland"};

    auto const wayland_surface_create_info = vk::WaylandSurfaceCreateInfoKHR{}
        .setDisplay(display)
        .setSurface(surface);

    vk_surface = ManagedResource<vk::SurfaceKHR>{
        vulkan->instance().createWaylandSurfaceKHR(wayland_surface_create_info),
        [this] (vk::SurfaceKHR& s) { vulkan->instance().destroySurfaceKHR(s); }};

    create_swapchain();

    vk_acquire_semaphore = ManagedResource<vk::Semaphore>{
        vulkan->device().createSemaphore(vk::SemaphoreCreateInfo()),
        [this] (auto& s) { vulkan->device().destroySemaphore(s); }};

}

void WaylandWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();
    vk_acquire_semaphore = {};
    vk_swapchain = {};
    vk_surface = {};
}

VulkanImage WaylandWindowSystem::next_vulkan_image()
{
    auto const image_index = vulkan->device().acquireNextImageKHR(
        vk_swapchain, UINT64_MAX, vk_acquire_semaphore, nullptr).value;
    
    return {image_index, vk_images[image_index], vk_acquire_semaphore};
}

void WaylandWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    auto const present_info = vk::PresentInfoKHR{}
        .setSwapchainCount(1)
        .setPSwapchains(&vk_swapchain.raw)
        .setPImageIndices(&vulkan_image.index)
        .setWaitSemaphoreCount(vulkan_image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&vulkan_image.semaphore);

    vulkan->graphics_queue().presentKHR(present_info);
}

bool WaylandWindowSystem::should_quit()
{
    while (wl_display_prepare_read(display) != 0)
        wl_display_dispatch_pending(display);

    if (wl_display_flush(display) < 0 && errno != EAGAIN)
    {
        wl_display_cancel_read(display);
        return should_quit_;
    }

    pollfd pfd{display_fd, POLLIN, 0};

    if (poll(&pfd, 1, 0) > 0)
    {
        wl_display_read_events(display);
        wl_display_dispatch_pending(display);
    }
    else
    {
        wl_display_cancel_read(display);
    }

    return should_quit_;
}

void WaylandWindowSystem::create_native_window()
{
    display = ManagedResource<wl_display*>{
        wl_display_connect(nullptr),
        [](auto d) { wl_display_flush(d); wl_display_disconnect(d); }};
    if (!display)
        throw std::runtime_error("Failed to connect to Wayland server");

    display_fd = wl_display_get_fd(display);

    wl_registry_listener const registry_listener = {
        WaylandWindowSystem::handle_registry_global,
        handle_registry_global_remove
    };

    auto const registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);

    surface = ManagedResource<wl_surface*>{
        wl_compositor_create_surface(compositor),
        wl_surface_destroy};
    if (!surface)
        throw std::runtime_error("Failed to create Wayland surface");

    shell_surface = ManagedResource<wl_shell_surface*>{
        wl_shell_get_shell_surface(shell, surface),
        wl_shell_surface_destroy};
    if (!shell_surface)
        throw std::runtime_error("Failed to create Wayland shell surface");

    wl_shell_surface_set_title(shell_surface, "vkmark " VKMARK_VERSION_STR);
    wl_shell_surface_set_toplevel(shell_surface);
}

void WaylandWindowSystem::create_swapchain()
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
        .setImageExtent({static_cast<uint32_t>(width), static_cast<uint32_t>(height)})
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setQueueFamilyIndexCount(1)
        .setPQueueFamilyIndices(&vulkan->graphics_queue_family_index())
        .setPresentMode(vk_present_mode);

    vk_swapchain = ManagedResource<vk::SwapchainKHR>{
        vulkan->device().createSwapchainKHR(swapchain_create_info),
        [this] (auto& s) { vulkan->device().destroySwapchainKHR(s); }};

    vk_images = vulkan->device().getSwapchainImagesKHR(vk_swapchain);
}

void WaylandWindowSystem::handle_registry_global(
    void* data, wl_registry* registry, uint32_t id,
    char const* interface_cstr, uint32_t version)
{
    auto const wws = static_cast<WaylandWindowSystem*>(data);
    auto const interface = std::string{interface_cstr ? interface_cstr : ""};

    if (interface == "wl_compositor")
    {
        auto compositor_raw = static_cast<wl_compositor*>(
            wl_registry_bind(registry, id, &wl_compositor_interface, 1));
        wws->compositor = ManagedResource<wl_compositor*>{
            std::move(compositor_raw), wl_compositor_destroy};
    }
    else if (interface == "wl_shell")
    {
        auto shell_raw = static_cast<wl_shell*>(
            wl_registry_bind(registry, id, &wl_shell_interface, 1));
        wws->shell = ManagedResource<wl_shell*>{
            std::move(shell_raw), wl_shell_destroy};
    }
    else if (interface == "wl_seat")
    {
        auto seat_raw = static_cast<wl_seat*>(
            wl_registry_bind(registry, id, &wl_seat_interface, 1));
        wws->seat = ManagedResource<wl_seat*>{
            std::move(seat_raw), wl_seat_destroy};

        wl_seat_add_listener(wws->seat, &seat_listener, wws);
    }
}

void WaylandWindowSystem::handle_seat_capabilities(
    void* data, wl_seat* seat, uint32_t capabilities)
{
    auto const wws = static_cast<WaylandWindowSystem*>(data);

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !wws->keyboard)
    {
        wws->keyboard = ManagedResource<wl_keyboard*>{
            wl_seat_get_keyboard(seat),
            wl_keyboard_destroy};

        wl_keyboard_add_listener(wws->keyboard, &wws->keyboard_listener, wws);

    }
    else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && wws->keyboard)
    {
        wws->keyboard = {};
    }
}

void WaylandWindowSystem::handle_keyboard_key(
    void* data, wl_keyboard* /*wl_keyboard*/,
    uint32_t /*serial*/, uint32_t /*time*/,
    uint32_t key, uint32_t state)
{
    if (key == KEY_ESC && state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        auto const wws = static_cast<WaylandWindowSystem*>(data);
        wws->should_quit_ = true;
    }
}

extern "C" int vkmark_window_system_probe()
{
    auto const display = wl_display_connect(nullptr);
    auto const connected = display != nullptr;

    if (connected) wl_display_disconnect(display);

    return connected ? 255 : 0;
}

extern "C" std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    return std::make_unique<WaylandWindowSystem>(
        options.size.first,
        options.size.second,
        options.present_mode);
}
