/*
 * Copyright © 2011 Linaro Ltd.
 * Copyright © 2017 Collabora Ltd.
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vkmark.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

#pragma once

#include <string>
#include <vector>
#include <utility>

#include <vulkan/vulkan.hpp>

struct Options
{
    Options();
    bool parse_args(int argc, char **argv);
    void print_help();

    std::vector<std::string> benchmarks;
    std::pair<int,int> size;
    vk::PresentModeKHR present_mode;
    bool list_scenes;
    bool show_all_options;
    std::string window_system_dir;
    std::string data_dir;
    bool show_debug;
    bool show_help;
};
