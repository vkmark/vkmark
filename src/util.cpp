/*
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

#include <sstream>
#include <sys/time.h>

#include "util.h"

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
