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

#include "catch.hpp"

using namespace Catch::Matchers;

namespace
{

struct TemporarySetDataDir
{
    TemporarySetDataDir(std::string const& dir) { Util::set_data_dir(dir); }
    ~TemporarySetDataDir() { Util::set_data_dir({}); }
};

}

SCENARIO("util data file", "")
{
    TemporarySetDataDir set_data_dir{VKMARK_TEST_DATA_DIR};

    GIVEN("An non-existent file")
    {
        std::string const non_existent_file{"non_existent.txt"};

        WHEN("reading the file")
        {
            THEN("an exception is thrown")
            {
                REQUIRE_THROWS(Util::read_data_file(non_existent_file));
            }
        }
    }

    GIVEN("A normal file")
    {
        std::string const normal_file{"test.txt"};

        WHEN("reading the file")
        {
            auto const contents = Util::read_data_file(normal_file);

            THEN("the file contents are returned")
            {
                REQUIRE_THAT(
                    contents,
                    Equals(std::vector<char>{'t','1','2','3','\n'}));
            }
        }
    }

    GIVEN("A file in subdirectory")
    {
        std::string const normal_file{"subdir/test.txt"};

        WHEN("reading the file")
        {
            auto const contents = Util::read_data_file(normal_file);

            THEN("the file contents are returned")
            {
                REQUIRE_THAT(
                    contents,
                    Equals(std::vector<char>{'s','1','2','3','\n'}));
            }
        }
    }
}

SCENARIO("util data file path", "")
{
    GIVEN("A set data dir")
    {
        std::string const data_dir = "/my/data/dir";
        TemporarySetDataDir set_data_dir{data_dir};

        WHEN("getting a data file path")
        {
            std::string const data_file = "subdir/bla.txt";
            auto const path = Util::get_data_file_path(data_file);

            THEN("the correct path is returned")
            {
                REQUIRE(path == data_dir + "/" + data_file);
            }
        }
    }

    GIVEN("An unset data dir")
    {
        WHEN("getting a data file path")
        {
            THEN("an exception is thrown")
            {
                REQUIRE_THROWS(Util::get_data_file_path("file"));
            }
        }
    }

    GIVEN("An empty data dir")
    {
        TemporarySetDataDir set_data_dir{""};

        WHEN("getting a data file path")
        {
            THEN("an exception is thrown")
            {
                REQUIRE_THROWS(Util::get_data_file_path("file"));
            }
        }
    }
}
