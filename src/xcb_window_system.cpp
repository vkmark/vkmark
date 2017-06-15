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

#include "xcb_window_system.h"
#include "options.h"

#include "vulkan_state.h"
#include "vulkan_image.h"

#include <xcb/xcb_icccm.h>
#include <stdexcept>

XcbWindowSystem::XcbWindowSystem(
    int width, int height, vk::PresentModeKHR present_mode)
    : requested_size{width, height},
      vk_present_mode{present_mode},
      size{width, height},
      connection{nullptr},
      window{XCB_NONE},
      root_visual{XCB_NONE},
      atom_wm_protocols{XCB_NONE},
      atom_wm_delete_window{XCB_NONE},
      vulkan{nullptr}
{
    create_native_window();
}

XcbWindowSystem::~XcbWindowSystem()
{
    xcb_unmap_window(connection, window);
    xcb_disconnect(connection);
}

std::vector<char const*> XcbWindowSystem::vulkan_extensions()
{
    return {VK_KHR_XCB_SURFACE_EXTENSION_NAME};
}

void XcbWindowSystem::init_vulkan(VulkanState& vulkan_)
{
    vulkan = &vulkan_;

    auto const vk_supported = vulkan->physical_device().getXcbPresentationSupportKHR(
        vulkan->graphics_queue_family_index(),
        connection,
        root_visual);

    if (!vk_supported)
        throw std::runtime_error{"Queue family does not support presentation on XCB"};

    auto const xcb_surface_create_info = vk::XcbSurfaceCreateInfoKHR{}
        .setConnection(connection)
        .setWindow(window);

    vk_surface = ManagedResource<vk::SurfaceKHR>{
        vulkan->instance().createXcbSurfaceKHR(xcb_surface_create_info),
        [this] (vk::SurfaceKHR& s) { vulkan->instance().destroySurfaceKHR(s); }};

    create_swapchain();

    vk_acquire_semaphore = ManagedResource<vk::Semaphore>{
        vulkan->device().createSemaphore(vk::SemaphoreCreateInfo()),
        [this] (auto& s) { vulkan->device().destroySemaphore(s); }};

}

void XcbWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();
    vk_acquire_semaphore = {};
    vk_swapchain = {};
    vk_surface = {};
}

VulkanImage XcbWindowSystem::next_vulkan_image()
{
    auto const image_index = vulkan->device().acquireNextImageKHR(
        vk_swapchain, UINT64_MAX, vk_acquire_semaphore, nullptr).value;
    
    return {image_index, vk_images[image_index], vk_image_format, vk_acquire_semaphore};
}

void XcbWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    auto const present_info = vk::PresentInfoKHR{}
        .setSwapchainCount(1)
        .setPSwapchains(&vk_swapchain.raw)
        .setPImageIndices(&vulkan_image.index)
        .setWaitSemaphoreCount(vulkan_image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&vulkan_image.semaphore);

    vulkan->graphics_queue().presentKHR(present_info);
}

std::vector<VulkanImage> XcbWindowSystem::vulkan_images()
{
    std::vector<VulkanImage> vulkan_images;

    for (uint32_t i = 0; i < vk_images.size(); ++i)
        vulkan_images.push_back({i, vk_images[i], vk_image_format, {}});

    return vulkan_images;
}

bool XcbWindowSystem::should_quit()
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

void XcbWindowSystem::create_native_window()
{
    static std::string const title{"vkmark " VKMARK_VERSION_STR};

    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection))
        throw std::runtime_error("Failed to connect to X server");

    window = xcb_generate_id(connection);

    uint32_t const window_values[] = { XCB_EVENT_MASK_KEY_PRESS };

    auto const iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
    auto const screen = iter.data;
    root_visual = screen->root_visual;

    if (fullscreen_requested())
    {
        size = {static_cast<int>(screen->width_in_pixels),
                static_cast<int>(screen->height_in_pixels)};
    }
    else
    {
        size = requested_size;
    }

    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        window,
        iter.data->root,
        0, 0,
        size.width,
        size.height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        iter.data->root_visual,
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
        xcb_size_hints_t size_hints;
        xcb_icccm_size_hints_set_min_size(&size_hints, size.width, size.height);
        xcb_icccm_size_hints_set_max_size(&size_hints, size.width, size.height);
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

void XcbWindowSystem::create_swapchain()
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
        .setImageExtent({static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height)})
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

xcb_atom_t XcbWindowSystem::atom(std::string const& name)
{
    auto const cookie = xcb_intern_atom(connection, 0, name.size(), name.c_str());
    auto const reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    auto const ret = reply ? reply->atom : XCB_NONE;

    free(reply);

    return ret;
}

bool XcbWindowSystem::fullscreen_requested()
{
    return requested_size.width == -1 && requested_size.height == -1;
}

extern "C" int vkmark_window_system_probe()
{
    auto const connection = xcb_connect(nullptr, nullptr);
    auto const has_error = xcb_connection_has_error(connection);
    xcb_disconnect(connection);

    return has_error ? 0 : 127;
}

extern "C" std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    return std::make_unique<XcbWindowSystem>(
        options.size.first,
        options.size.second,
        options.present_mode);
}
