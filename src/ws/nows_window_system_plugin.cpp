/*
 * Copyright © 2024 Igalia S.L.
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
 *   Lucas Fryzek <lfryzek@igalia.com>
 */

#include "window_system_plugin.h"
#include "nows_window_system.h"

#include "options.h"

std::string const width_opt{"width"};
std::string const height_opt{"height"};
std::string const num_buffers_opt{"num-buffers"};

static uint32_t get_int_option(Options const& options, std::string const& option_name, uint32_t default_value)
{
    auto const& winsys_options = options.window_system_options;

    for (auto const& opt : winsys_options)
    {
        if (opt.name == option_name)
        {
            try {
                return std::stoi(opt.value);
            } catch ( ... ) {
                throw std::runtime_error{"Invalid value '" + opt.value + "' for option '" + option_name + "'"};
            }
        }
    }

    return default_value;
}

void vkmark_window_system_load_options(Options& options)
{
    options.add_window_system_help(
        "No window system options (pass in --winsys-options)\n"
        "  width=X          Buffer width to use\n"
        "  height=X         Buffer height to use\n"
        "  num-buffers=X    Number of offscreen buffers to allocate\n"
        );
}

int vkmark_window_system_probe(Options const&)
{
    return 1;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    auto pixel_format = options.pixel_format;
    if (options.pixel_format == vk::Format::eUndefined)
        pixel_format = vk::Format::eR8G8B8A8Srgb;
    return std::make_unique<NoWindowSystem>(
            pixel_format,
            get_int_option(options, width_opt, 512),
            get_int_option(options, height_opt, 512),
            get_int_option(options, num_buffers_opt, 3));
}
