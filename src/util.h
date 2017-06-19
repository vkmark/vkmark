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

#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

namespace Util
{

std::vector<std::string> split(std::string const& src, char delim);

uint64_t get_timestamp_us();

void set_data_dir(std::string const& path);
std::vector<char> read_data_file(std::string const& rel_path);

template<typename T>
T from_string(std::string const& str)
{
    std::stringstream ss{str};
    T ret{};
    ss >> ret;
    return ret;
}

template <typename Init, typename Deinit>
struct RAIIHelper
{
    RAIIHelper(Init&& init, Deinit&& deinit) : deinit{deinit} { init(); }
    ~RAIIHelper() { deinit(); }

    Deinit deinit;
};

template <typename Init, typename Deinit>
RAIIHelper<Init,Deinit> make_raii(Init&& init, Deinit&& deinit)
{
    return {std::move(init), std::move(deinit)};
}

}
