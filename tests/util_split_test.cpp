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

SCENARIO("util split", "")
{
    GIVEN("An empty string")
    {
        std::string empty;

        WHEN("splitting with a delimeter")
        {
            auto const elements = Util::split(empty, ':');

            THEN("no elements are returned")
            {
                REQUIRE(elements.empty());
            }
        }
    }

    GIVEN("A string with empty elements")
    {
        std::string str{":::"};

        WHEN("splitting with : delimeter")
        {
            auto const elements = Util::split(str, ':');

            THEN("all empty elements are returned")
            {
                REQUIRE_THAT(
                    elements,
                    Equals(std::vector<std::string>{4, ""}));
            }

        }
    }

    GIVEN("A string with non-empty elements")
    {
        std::string str{"aa:bb:cc"};

        WHEN("splitting with correct delimeter")
        {
            auto const elements = Util::split(str, ':');

            THEN("all elements are returned")
            {
                REQUIRE_THAT(
                    elements,
                    Equals(std::vector<std::string>{"aa", "bb", "cc"}));
            }

        }

        WHEN("splitting with incorrect delimeter")
        {
            auto const elements = Util::split(str, '=');

            THEN("a single element is returned")
            {
                REQUIRE_THAT(
                    elements,
                    Equals(std::vector<std::string>{"aa:bb:cc"}));
            }
        }
    }

    GIVEN("A string with empty element at beginning")
    {
        std::string str{":aa"};

        WHEN("splitting with correct delimeter")
        {
            auto const elements = Util::split(str, ':');

            THEN("all elements are returned")
            {
                REQUIRE_THAT(
                    elements,
                    Equals(std::vector<std::string>{"", "aa"}));
            }
        }
    }

    GIVEN("A string with empty element at end")
    {
        std::string str{"aa:"};

        WHEN("splitting with correct delimeter")
        {
            auto const elements = Util::split(str, ':');

            THEN("all elements are returned")
            {
                REQUIRE_THAT(
                    elements,
                    Equals(std::vector<std::string>{"aa", ""}));
            }
        }
    }
}
