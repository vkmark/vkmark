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

#include "window_system_plugin.h"
#include "swapchain_window_system.h"
#include "wayland_native_system.h"

#include "options.h"

#include <wayland-client.h>

#define VKMARK_WAYLAND_WINDOW_SYSTEM_PRIORITY 1

void vkmark_window_system_load_options(Options&)
{
}

int vkmark_window_system_probe(Options const&)
{
    struct wl_display* display;
    int score = 0;

    /* If we have an explicit and usable WAYLAND_DISPLAY,
     * Wayland is a good choice. */
    auto const display_env = getenv("WAYLAND_DISPLAY");
    if (display_env && (display = wl_display_connect(display_env)))
    {
        wl_display_disconnect(display);
        score = VKMARK_WINDOW_SYSTEM_PROBE_GOOD;
    }

    /* If we have a usable default connection, Wayland is
     * an ok choice, but there may be better ones. */
    if (!score && (display = wl_display_connect(nullptr)))
    {
        wl_display_disconnect(display);
        score = VKMARK_WINDOW_SYSTEM_PROBE_OK;
    }

    if (score)
        score += VKMARK_WAYLAND_WINDOW_SYSTEM_PRIORITY;

    return score;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    return std::make_unique<SwapchainWindowSystem>(
        std::make_unique<WaylandNativeSystem>(options.size.first, options.size.second),
        options.present_mode,
        options.pixel_format);
}
