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

#include "wayland_native_system.h"

#include "vulkan_state.h"

#include <stdexcept>
#include <linux/input.h>
#include <poll.h>

namespace
{

void handle_registry_global_remove(
    void* /*data*/, struct wl_registry* /*registry*/, uint32_t /*name*/)
{
}

void handle_output_geometry(
    void* /*data*/, wl_output* /*wl_output*/, int32_t /*x*/, int32_t /*y*/,
    int32_t /*physical_width*/, int32_t /*physical_height*/,
    int32_t /*subpixel*/, char const* /*make*/, char const* /*model*/,
    int32_t /*transform*/)
{
}

void handle_output_done(void* /*data*/, wl_output* /*wl_output*/)
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

void handle_xdg_wm_base_ping(
    void* /*data*/, xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void handle_xdg_surface_configure(
    void* /*data*/, xdg_surface* xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
}

void handle_xdg_toplevel_configure(
    void* /*data*/, struct xdg_toplevel* /*xdg_toplevel*/,
    int32_t /*width*/, int32_t /*height*/, wl_array* /*states*/)
{
}

}

xdg_wm_base_listener const WaylandNativeSystem::xdg_wm_base_listener{
    handle_xdg_wm_base_ping,
};

xdg_toplevel_listener const WaylandNativeSystem::xdg_toplevel_listener{
    handle_xdg_toplevel_configure,
    WaylandNativeSystem::handle_xdg_toplevel_close,
};

xdg_surface_listener const WaylandNativeSystem::xdg_surface_listener{
    handle_xdg_surface_configure,
};

wl_seat_listener const WaylandNativeSystem::seat_listener{
    WaylandNativeSystem::handle_seat_capabilities
};

wl_output_listener const WaylandNativeSystem::output_listener{
    handle_output_geometry,
    WaylandNativeSystem::handle_output_mode,
    handle_output_done,
    WaylandNativeSystem::handle_output_scale
};

wl_keyboard_listener const WaylandNativeSystem::keyboard_listener{
    handle_keyboard_keymap,
    handle_keyboard_enter,
    handle_keyboard_leave,
    WaylandNativeSystem::handle_keyboard_key,
    handle_keyboard_modifiers,
    handle_keyboard_repeat_info
};

WaylandNativeSystem::WaylandNativeSystem(int width, int height)
    : requested_width{width},
      requested_height{height},
      should_quit_{false},
      display_fd{0},
      output_width{0},
      output_height{0},
      output_refresh{0},
      output_scale{1}
{
    create_native_window();
}

std::vector<char const*> WaylandNativeSystem::instance_extensions()
{
    return {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
}

uint32_t WaylandNativeSystem::get_presentation_queue_family_index(vk::PhysicalDevice const& pd)
{
    auto const queue_families = pd.getQueueFamilyProperties();

    for (auto i = 0u; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueCount > 0 &&
            pd.getWaylandPresentationSupportKHR(i, display))
        {
            return i;
        }
    }

    return invalid_queue_family_index;
}

bool WaylandNativeSystem::should_quit()
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

vk::Extent2D WaylandNativeSystem::get_vk_extent()
{
    return vk_extent;
}

ManagedResource<vk::SurfaceKHR> WaylandNativeSystem::create_vk_surface(VulkanState& vulkan)
{
    auto const wayland_surface_create_info = vk::WaylandSurfaceCreateInfoKHR{}
        .setDisplay(display)
        .setSurface(surface);

    return ManagedResource<vk::SurfaceKHR>{
        vulkan.instance().createWaylandSurfaceKHR(wayland_surface_create_info),
        [vptr=&vulkan] (vk::SurfaceKHR& s) { vptr->instance().destroySurfaceKHR(s); }};
}

void WaylandNativeSystem::create_native_window()
{
    display = ManagedResource<wl_display*>{
        wl_display_connect(nullptr),
        [](auto d) { wl_display_flush(d); wl_display_disconnect(d); }};
    if (!display)
        throw std::runtime_error("Failed to connect to Wayland server");

    display_fd = wl_display_get_fd(display);

    wl_registry_listener const registry_listener = {
        WaylandNativeSystem::handle_registry_global,
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

    if (!xdg_wm_base)
    {
        throw std::runtime_error(
            "Failed to create Wayland xdg_surface, xdg_wm_base not supported");
    }

    xdg_surface = ManagedResource<struct xdg_surface*>{
        xdg_wm_base_get_xdg_surface(xdg_wm_base, surface),
        xdg_surface_destroy};
    if (!xdg_surface)
        throw std::runtime_error("Failed to create Wayland xdg_surface");

    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this);

    xdg_toplevel = ManagedResource<struct xdg_toplevel*>{
        xdg_surface_get_toplevel(xdg_surface),
        xdg_toplevel_destroy};
    if (!xdg_toplevel)
        throw std::runtime_error("Failed to create Wayland xdg_toplevel");

    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, this);

    xdg_toplevel_set_app_id(xdg_toplevel, "com.github.vkmark.vkmark");
    xdg_toplevel_set_title(xdg_toplevel, "vkmark " VKMARK_VERSION_STR);

    if (fullscreen_requested())
    {
        xdg_toplevel_set_fullscreen(xdg_toplevel, output);
        vk_extent = vk::Extent2D{
            static_cast<uint32_t>(output_width),
            static_cast<uint32_t>(output_height)};
    }
    else
    {
        vk_extent = vk::Extent2D{
            static_cast<uint32_t>(requested_width),
            static_cast<uint32_t>(requested_height)};
    }

    if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(output.raw)) >=
        WL_OUTPUT_SCALE_SINCE_VERSION)
    {
        wl_surface_set_buffer_scale(surface, output_scale);
    }

    auto const opaque_region = ManagedResource<wl_region*>{
        wl_compositor_create_region(compositor), wl_region_destroy};
    wl_region_add(opaque_region, 0, 0, vk_extent.width, vk_extent.height);
    wl_surface_set_opaque_region(surface, opaque_region);

    wl_surface_commit(surface);
}

bool WaylandNativeSystem::fullscreen_requested()
{
    return requested_width == -1 && requested_height == -1;
}

void WaylandNativeSystem::handle_registry_global(
    void* data, wl_registry* registry, uint32_t id,
    char const* interface_cstr, uint32_t version)
{
    auto const wws = static_cast<WaylandNativeSystem*>(data);
    auto const interface = std::string{interface_cstr ? interface_cstr : ""};

    if (interface == "wl_compositor")
    {
        auto compositor_raw = static_cast<wl_compositor*>(
            wl_registry_bind(registry, id, &wl_compositor_interface,
                std::min(version, 4U)));
        wws->compositor = ManagedResource<wl_compositor*>{
            std::move(compositor_raw), wl_compositor_destroy};
    }
    else if (interface == "xdg_wm_base")
    {
        auto xdg_wm_base_raw = static_cast<struct xdg_wm_base*>(
            wl_registry_bind(
                registry, id, &xdg_wm_base_interface, std::min(version, 2U)));
        wws->xdg_wm_base = ManagedResource<struct xdg_wm_base*>{
            std::move(xdg_wm_base_raw), xdg_wm_base_destroy};
        xdg_wm_base_add_listener(wws->xdg_wm_base, &xdg_wm_base_listener, wws);
    }
    else if (interface == "wl_seat")
    {
        auto seat_raw = static_cast<wl_seat*>(
            wl_registry_bind(registry, id, &wl_seat_interface, 1));
        wws->seat = ManagedResource<wl_seat*>{
            std::move(seat_raw), wl_seat_destroy};

        wl_seat_add_listener(wws->seat, &seat_listener, wws);
    }
    else if (interface == "wl_output")
    {
        if (!wws->output)
        {
            auto output_raw = static_cast<wl_output*>(
                wl_registry_bind(registry, id, &wl_output_interface,
                    std::min(version, 2U)));
            wws->output = ManagedResource<wl_output*>{
                std::move(output_raw), wl_output_destroy};

            wl_output_add_listener(wws->output, &output_listener, wws);
            wl_display_roundtrip(wws->display);
        }
    }
}

void WaylandNativeSystem::handle_xdg_toplevel_close(
    void* data, struct xdg_toplevel* /*xdg_toplevel*/)
{
    auto const wws = static_cast<WaylandNativeSystem*>(data);
    wws->should_quit_ = true;
}

void WaylandNativeSystem::handle_seat_capabilities(
    void* data, wl_seat* seat, uint32_t capabilities)
{
    auto const wws = static_cast<WaylandNativeSystem*>(data);

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

void WaylandNativeSystem::handle_output_mode(
    void* data, wl_output* output,
    uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    if (flags & WL_OUTPUT_MODE_CURRENT)
    {
        auto const wws = static_cast<WaylandNativeSystem*>(data);
        wws->output_width = width;
        wws->output_height = height;
        wws->output_refresh = refresh;
    }
}

void WaylandNativeSystem::handle_output_scale(
    void* data, wl_output* /*output*/, int32_t factor)
{
    auto const wws = static_cast<WaylandNativeSystem*>(data);
    wws->output_scale = factor;
}

void WaylandNativeSystem::handle_keyboard_key(
    void* data, wl_keyboard* /*wl_keyboard*/,
    uint32_t /*serial*/, uint32_t /*time*/,
    uint32_t key, uint32_t state)
{
    if (key == KEY_ESC && state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        auto const wws = static_cast<WaylandNativeSystem*>(data);
        wws->should_quit_ = true;
    }
}
