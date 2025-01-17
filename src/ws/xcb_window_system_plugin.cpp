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
#include "xcb_native_system.h"
#include "window_system_priority.h"

#include "options.h"
#include "log.h"

#include <xcb/xcb.h>

namespace
{

std::string const visual_id_opt{"xcb-visual-id"};

}

void vkmark_window_system_load_options(Options& options)
{
    options.add_window_system_help(
        "XCB window system options (pass in --winsys-options)\n"
        "  xcb-visual-id=ID            The X11 visual to use in hex (default: root)\n"
        );
}

int vkmark_window_system_probe(Options const&)
{
    xcb_connection_t* connection;
    int score = 0;

    /* If we have an explicit and usable DISPLAY, X11 is a good choice. */
    auto const display_env = getenv("DISPLAY");
    if (display_env && (connection = xcb_connect(display_env, nullptr)))
    {
        if (!xcb_connection_has_error(connection))
            score = VKMARK_WINDOW_SYSTEM_PROBE_GOOD;
        xcb_disconnect(connection);
    }

    /* If we have a usable default connection, XCB is
     * an ok choice, but there may be better ones. */
    if (!score && (connection = xcb_connect(nullptr, nullptr)))
    {
        if (!xcb_connection_has_error(connection))
            score = VKMARK_WINDOW_SYSTEM_PROBE_OK;
        xcb_disconnect(connection);
    }

    if (score)
        score += VKMARK_XCB_WINDOW_SYSTEM_PRIORITY;

    return score;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    auto const& winsys_options = options.window_system_options;
    xcb_visualid_t visual_id = XCB_NONE;

    for (auto const& opt : winsys_options)
    {
        if (opt.name == visual_id_opt)
        {
            visual_id = !opt.value.empty() ?
                        std::stoul(opt.value, nullptr, 16) :
                        XCB_NONE;
        }
        else
        {
            Log::info("XcbWindowSystemPlugin: Ignoring unknown window system option '%s'\n",
                      opt.name.c_str());
        }
    }

    return std::make_unique<SwapchainWindowSystem>(
        std::make_unique<XcbNativeSystem>(
            options.size.first, options.size.second, visual_id),
        options.present_mode,
        options.pixel_format);
}
