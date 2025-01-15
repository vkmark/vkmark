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

#include "xcb_native_system.h"

#include "vulkan_state.h"
#include "log.h"

#include <xcb/xcb_icccm.h>
#include <stdexcept>

XcbNativeSystem::XcbNativeSystem(
    int width, int height, xcb_visualid_t visual_id)
    : requested_width{width},
      requested_height{height},
      connection{nullptr},
      window{XCB_NONE},
      visual_id{visual_id},
      atom_wm_protocols{XCB_NONE},
      atom_wm_delete_window{XCB_NONE}
{
    create_native_window();
}

XcbNativeSystem::~XcbNativeSystem()
{
    xcb_unmap_window(connection, window);
    xcb_disconnect(connection);
}

VulkanWSI::Extensions XcbNativeSystem::required_extensions()
{
    return {{VK_KHR_SURFACE_EXTENSION_NAME,
             VK_KHR_XCB_SURFACE_EXTENSION_NAME},
            {}};
}

bool XcbNativeSystem::should_quit()
{
    bool quit = false;

    while (auto const event = xcb_poll_for_event(connection))
    {
        switch (event->response_type & 0x7f)
        {
        case XCB_KEY_PRESS:
            {
            auto const key_press = reinterpret_cast<xcb_key_press_event_t const*>(event);
            if (key_press->detail == 9)
                quit = true;
            break;
            }
        case XCB_CLIENT_MESSAGE:
            {
            auto const client_message = reinterpret_cast<xcb_client_message_event_t const*>(event);
            if (client_message->window == window &&
                client_message->type == atom_wm_protocols &&
                client_message->data.data32[0] == atom_wm_delete_window)
            {
                quit = true;
            }
            break;
            }
        default:
            break;
        }

        free(event);
    }

    return quit;
}

vk::Extent2D XcbNativeSystem::get_vk_extent()
{
    return vk_extent;
}

uint32_t XcbNativeSystem::get_presentation_queue_family_index(vk::PhysicalDevice const& pd)
{
    auto const queue_families = pd.getQueueFamilyProperties();

    for (auto i = 0u; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueCount > 0 &&
            pd.getXcbPresentationSupportKHR(i, connection, visual_id))
        {
            return i;
        }
    }

    return invalid_queue_family_index;
}

ManagedResource<vk::SurfaceKHR> XcbNativeSystem::create_vk_surface(VulkanState& vulkan)
{
    auto const xcb_surface_create_info = vk::XcbSurfaceCreateInfoKHR{}
        .setConnection(connection)
        .setWindow(window);

    return ManagedResource<vk::SurfaceKHR>{
        vulkan.instance().createXcbSurfaceKHR(xcb_surface_create_info),
        [vptr=&vulkan] (vk::SurfaceKHR& s) { vptr->instance().destroySurfaceKHR(s); }};
}


void XcbNativeSystem::create_native_window()
{
    static std::string const title{"vkmark " VKMARK_VERSION_STR};

    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection))
        throw std::runtime_error("Failed to connect to X server");

    window = xcb_generate_id(connection);

    uint32_t const window_values[] = { XCB_EVENT_MASK_KEY_PRESS };

    auto const iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
    auto const screen = iter.data;

    if (visual_id == XCB_NONE)
    {
        visual_id = screen->root_visual;
        Log::debug("XcbNativeSystem: Using root visual 0x%x for window\n",
                   visual_id);
    }
    else
    {
        Log::debug("XcbNativeSystem: Using user-specified visual 0x%x for window\n",
                   visual_id);
    }

    if (fullscreen_requested())
    {
        vk_extent = vk::Extent2D{
            static_cast<uint32_t>(screen->width_in_pixels),
            static_cast<uint32_t>(screen->height_in_pixels)};
    }
    else
    {
        vk_extent = vk::Extent2D{
            static_cast<uint32_t>(requested_width),
            static_cast<uint32_t>(requested_height)};
    }

    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        window,
        iter.data->root,
        0, 0,
        vk_extent.width,
        vk_extent.height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        visual_id,
        XCB_CW_EVENT_MASK, window_values);

    // Set window name
    xcb_icccm_set_wm_name(
        connection, window, atom("UTF8_STRING"), 8, title.size(), title.c_str());

    if (fullscreen_requested())
    {
        // Make window fullscreen
        auto const atom_fs = atom("_NET_WM_STATE_FULLSCREEN");
        xcb_change_property(
            connection, XCB_PROP_MODE_REPLACE, window,
            atom("_NET_WM_STATE"), XCB_ATOM_ATOM, 32, 1, &atom_fs);
    }
    else
    {
        // Make window non-resizable
        xcb_size_hints_t size_hints = {0};
        xcb_icccm_size_hints_set_min_size(&size_hints, vk_extent.width, vk_extent.height);
        xcb_icccm_size_hints_set_max_size(&size_hints, vk_extent.width, vk_extent.height);
        xcb_icccm_set_wm_normal_hints(connection, window, &size_hints);
    }

    // Set up window delete handling
    atom_wm_protocols = atom("WM_PROTOCOLS");
    atom_wm_delete_window = atom("WM_DELETE_WINDOW");
    xcb_icccm_set_wm_protocols(
        connection, window, atom_wm_protocols, 1, &atom_wm_delete_window);

    xcb_map_window(connection, window);

    xcb_flush(connection);
}

xcb_atom_t XcbNativeSystem::atom(std::string const& name)
{
    auto const cookie = xcb_intern_atom(connection, 0, name.size(), name.c_str());
    auto const reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    auto const ret = reply ? reply->atom : XCB_NONE;

    free(reply);

    return ret;
}

bool XcbNativeSystem::fullscreen_requested()
{
    return requested_width == -1 && requested_height == -1;
}
