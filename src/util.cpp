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

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"

namespace
{
std::string data_dir;
}

std::vector<std::string> Util::split(std::string const& src, char delim)
{
    std::vector<std::string> elements;
    std::stringstream ss{src, std::ios_base::ate | std::ios_base::in | std::ios_base::out};

    // std::getline() doesn't deal with trailing delimiters in the
    // way we want it to (i.e. "a:b:" => {"a","b",""}), so fix this
    // by appending an extra delimiter in such cases.
    if (!src.empty() && src.back() == delim)
        ss.put(delim);

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

std::string Util::get_data_file_path(std::string const& rel_path)
{
    if (data_dir.empty())
        throw std::logic_error("Data directory not set!");

    return data_dir + "/" + rel_path;
}

std::vector<char> Util::read_data_file(std::string const& rel_path)
{
    auto const path = get_data_file_path(rel_path);
    std::ifstream ifs{path, std::ios::ate | std::ios::binary};

    if (!ifs)
        throw std::runtime_error{"Failed to open file " + path};

    auto const file_size = ifs.tellg();
    std::vector<char> buffer(file_size);

    ifs.seekg(0);
    ifs.read(buffer.data(), file_size);

    return buffer;
}

Util::Image::Image()
    : data{nullptr}, size{0}, width{0}, height{0}
{
}

Util::Image::~Image()
{
    if (data)
        stbi_image_free(data);
}

Util::Image::Image(Image&& other)
    : data{other.data},
      size{other.size},
      width{other.width},
      height{other.height}
{
    other.data = nullptr;
}

Util::Image& Util::Image::operator=(Image&& other)
{
    if (data)
        stbi_image_free(data);
    data = other.data;
    size = other.size;
    width = other.width;
    height = other.height;

    other.data = nullptr;

    return *this;
}

Util::Image Util::read_image_file(std::string const& rel_path)
{
    auto const path = get_data_file_path(rel_path);

    int w = 0;
    int h = 0;
    int c = 0;

    Image image;
    image.data = stbi_load(path.c_str(), &w, &h, &c, STBI_rgb_alpha);

    if (!image.data)
    {
        throw std::runtime_error{
            "Failed to read image file " + path + ": " + stbi_failure_reason()};
    }

    image.width = static_cast<size_t>(w);
    image.height = static_cast<size_t>(h);
    image.size = image.width * image.height * 4;

    return image;
}
