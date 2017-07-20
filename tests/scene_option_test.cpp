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

#include "src/scene.h"

#include "catch.hpp"

SCENARIO("scene option acceptable values", "")
{
    GIVEN("A SceneOption with undefined acceptable values")
    {
        SceneOption option{"name", "val1", "description"};

        WHEN("querying whether the option accepts a value")
        {
            THEN("all values are accepted")
            {
                REQUIRE(option.accepts_value("val1"));
                REQUIRE(option.accepts_value("arbitrary_val"));
                REQUIRE(option.accepts_value(""));
            }
        }
    }

    GIVEN("A SceneOption with multiple acceptable values")
    {
        SceneOption option{"name", "val1", "description", "val1,val3,val5"};

        WHEN("querying whether the option accepts a value")
        {
            THEN("all acceptable values are accepted")
            {
                REQUIRE(option.accepts_value("val1"));
                REQUIRE(option.accepts_value("val3"));
                REQUIRE(option.accepts_value("val5"));
            }

            THEN("non-acceptable values are not accepted")
            {
                REQUIRE_FALSE(option.accepts_value(""));
                REQUIRE_FALSE(option.accepts_value("val2"));
                REQUIRE_FALSE(option.accepts_value("val4"));
            }
        }
    }
}
