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

#include "src/util.h"

#include "noise_rgb.c"
#include "noise_rgba.c"

#include "catch.hpp"

#include <cstring>

namespace
{

struct TemporarySetDataDir
{
    TemporarySetDataDir(std::string const& dir) { Util::set_data_dir(dir); }
    ~TemporarySetDataDir() { Util::set_data_dir({}); }
};

template <typename ImageStruct>
class ImageEquals : public Catch::MatcherBase<Util::Image>
{
public:
    ImageEquals(ImageStruct& image_struct) : image_struct{image_struct} {}

    virtual bool match(Util::Image const& image) const override
    {
        return image.width == image_struct.width &&
               image.height == image_struct.height &&
               image.size == (sizeof(image_struct.pixel_data) - 1) &&
               !std::memcmp(image.data, image_struct.pixel_data, image.size);
    }

    virtual std::string describe() const override
    {
        std::ostringstream ss;
        ss << "matches image with width:" << image_struct.width
           << " height:" << image_struct.height
           << " size:" << sizeof(image_struct.pixel_data) - 1;
        return ss.str();
    }

    ImageStruct& image_struct;
};

template<typename ImageStruct>
ImageEquals<ImageStruct> Equals(ImageStruct& image_struct)
{
    return ImageEquals<ImageStruct>(image_struct);
}

}

SCENARIO("util image file reads", "")
{
    TemporarySetDataDir set_data_dir{VKMARK_TEST_DATA_DIR};

    GIVEN("An non-existent image file")
    {
        std::string const non_existent_file{"non_existent.txt"};

        WHEN("reading the file")
        {
            THEN("an exception is thrown")
            {
                REQUIRE_THROWS(Util::read_image_file(non_existent_file));
            }
        }
    }

    GIVEN("An image file without alpha")
    {
        std::string const rgb_file{"images/noise-rgb.png"};

        WHEN("reading the file")
        {
            auto const image = Util::read_image_file(rgb_file);

            THEN("the image contents are returned with fixed alpha 0xff")
            {
                REQUIRE_THAT(image, Equals(noise_rgb_image));
            }
        }
    }

    GIVEN("An image file with alpha")
    {
        std::string const rgba_file{"images/noise-rgba.png"};

        WHEN("reading the file")
        {
            auto const image = Util::read_image_file(rgba_file);

            THEN("the image contents are returned with alpha")
            {
                REQUIRE_THAT(image, Equals(noise_rgba_image));
            }
        }
    }
}

SCENARIO("util image object", "")
{
    TemporarySetDataDir set_data_dir{VKMARK_TEST_DATA_DIR};

    GIVEN("An image object")
    {
        auto image = Util::read_image_file("images/noise-rgb.png");
        auto const image_data = image.data;
        auto const image_size = image.size;
        auto const image_width = image.width;
        auto const image_height = image.width;

        WHEN("move constructing a new image object")
        {
            Util::Image new_image{std::move(image)};

            THEN("the old image object is emptied")
            {
                REQUIRE(image.data == nullptr);
            }

            THEN("the new image object contains valid data")
            {
                REQUIRE(new_image.data == image_data);
                REQUIRE(new_image.size == image_size);
                REQUIRE(new_image.width == image_width);
                REQUIRE(new_image.height == image_height);
            }
        }

        WHEN("move assigning to an image object")
        {
            Util::Image new_image;
            new_image = std::move(image);

            THEN("the old image object is emptied")
            {
                REQUIRE(image.data == nullptr);
            }

            THEN("the new image object contains valid data")
            {
                REQUIRE(new_image.data == image_data);
                REQUIRE(new_image.size == image_size);
                REQUIRE(new_image.width == image_width);
                REQUIRE(new_image.height == image_height);
            }
        }
    }
}
