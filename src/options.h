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

#include <optional>
#include <string>
#include <vector>
#include <utility>

#include <vulkan/vulkan.hpp>

#include "device_uuid.h"

struct Options
{
    struct WindowSystemOption
    {
        std::string name;
        std::string value;
    };

    Options();
    bool parse_args(int argc, char **argv);
    std::string help_string();

    void add_window_system_help(std::string const& help);

    std::vector<std::string> benchmarks;
    std::pair<int,int> size;
    vk::PresentModeKHR present_mode;
    vk::Format pixel_format;
    bool list_scenes;
    bool show_all_options;
    std::string window_system_dir;
    std::string data_dir;
    std::string window_system;
    std::vector<WindowSystemOption> window_system_options;
    bool run_forever;
    bool show_debug;
    bool show_help;
    bool list_devices;
    std::optional<DeviceUUID> use_device_with_uuid;

private:
    std::vector<std::string> window_system_help;
};
