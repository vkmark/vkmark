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

#include <cstdio>
#include <getopt.h>

#include "options.h"
#include "util.h"

namespace
{

struct option long_options[] = {
    {"benchmark", 1, 0, 0},
    {"size", 1, 0, 0},
    {"present-mode", 1, 0, 0},
    {"list-scenes", 0, 0, 0},
    {"show-all-options", 0, 0, 0},
    {"window-system-dir", 1, 0, 0},
    {"debug", 0, 0, 0},
    {"help", 0, 0, 0},
    {0, 0, 0, 0}
};

std::pair<int,int> parse_size(std::string const& str)
{
    std::pair<int,int> size;
    auto const dimensions = Util::split(str, 'x');

    size.first = Util::from_string<int>(dimensions[0]);

    /*
     * Parse the second element (height). If there is none, use the value
     * of the first element for the second (width = height)
     */
    if (dimensions.size() > 1)
        size.second = Util::from_string<int>(dimensions[1]);
    else
        size.second = size.first;

    return size;
}

vk::PresentModeKHR parse_present_mode(std::string const& str)
{
    if (str == "immediate")
        return vk::PresentModeKHR::eImmediate;
    else if (str == "mailbox")
        return vk::PresentModeKHR::eMailbox;
    else if (str == "fifo")
        return vk::PresentModeKHR::eFifo;
    else if (str == "fiforelaxed")
        return vk::PresentModeKHR::eFifoRelaxed;
    else
        return vk::PresentModeKHR::eMailbox;
}

}

Options::Options()
    : size{800, 600},
      present_mode{vk::PresentModeKHR::eMailbox},
      list_scenes{false},
      show_all_options{false},
      window_system_dir{VKMARK_WINDOW_SYSTEM_DIR},
      show_debug{false},
      show_help{false}
{
}

void Options::print_help()
{
    printf("A benchmark for Vulkan\n"
           "\n"
           "Options:\n"
           "  -b, --benchmark BENCH       A benchmark to run: 'scene(:opt1=val1)*'\n"
           "                              (the option can be used multiple times)\n"
           "  -s, --size WxH              Size of the output window (default: 800x600)\n"
           "  -p, --present-mode PM       Vulkan present mode (default: mailbox)\n"
           "                              [immediate, mailbox, fifo, fiforelaxed]\n"
           "  -l, --list-scenes           Display information about the available scenes\n"
           "                              and their options\n"
           "      --show-all-options      Show all scene option values used for benchmarks\n"
           "                              (only explicitly set options are shown by default)\n"
           "      --window-system-dir DIR Directory to search in for window system modules\n"
           "                              (only explicitly set options are shown by default)\n"
           "  -d, --debug                 Display debug messages\n"
           "  -h, --help                  Display help\n");
}

bool Options::parse_args(int argc, char **argv)
{
    while (true)
    {
        int option_index = -1;
        int c;
        std::string optname;

        c = getopt_long(argc, argv, "b:s:p:ldh",
                        long_options, &option_index);
        if (c == -1)
            break;
        if (c == ':' || c == '?')
            return false;

        if (option_index != -1)
            optname = long_options[option_index].name;

        if (c == 'b' || optname == "benchmark")
            benchmarks.push_back(optarg);
        else if (c == 's' || optname == "size")
            size = parse_size(optarg);
        else if (c == 'p' || optname == "present-mode")
            present_mode = parse_present_mode(optarg);
        else if (c == 'l' || optname == "list-scenes")
            list_scenes = true;
        else if (optname == "show-all-options")
            show_all_options = true;
        else if (optname == "window-system-dir")
            window_system_dir = optarg;
        else if (c == 'd' || optname == "debug")
            show_debug = true;
        else if (c == 'h' || optname == "help")
            show_help = true;
    }

    return true;
}
