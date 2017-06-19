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

#include <sstream>
#include <fstream>
#include <sys/time.h>

#include "util.h"
#include <stdexcept>

namespace
{
std::string data_dir;
}

std::vector<std::string> Util::split(std::string const& src, char delim)
{
    std::vector<std::string> elements;
    std::stringstream ss{src};

    std::string item;
    while(std::getline(ss, item, delim))
        elements.push_back(item);

    return elements;
}

uint64_t Util::get_timestamp_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t const now = static_cast<uint64_t>(ts.tv_sec) * 1000000 +
                         static_cast<uint64_t>(ts.tv_nsec) / 1000;
    return now;
}

void Util::set_data_dir(std::string const& dir)
{
    data_dir = dir;
}

std::vector<char> Util::read_data_file(std::string const& rel_path)
{
    if (data_dir.empty())
        throw std::logic_error("Data directory not set!");

    auto const path = data_dir + "/" + rel_path;
    std::ifstream ifs{path, std::ios::ate | std::ios::binary};

    if (!ifs)
        throw std::runtime_error{"Failed to open file " + path};

    auto const file_size = ifs.tellg();
    std::vector<char> buffer(file_size);

    ifs.seekg(0);
    ifs.read(buffer.data(), file_size);

    return buffer;
}
