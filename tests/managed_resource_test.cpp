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

#include "src/managed_resource.h"

#include "catch.hpp"

#include <memory>

SCENARIO("managed resource", "")
{
    GIVEN("A managed resource with a destructor")
    {
        int x = 0;
        auto mr = std::make_unique<ManagedResource<int*>>(
            std::move(&x), [] (auto const& p) { *p = -1; });

        WHEN("the managed resource is destroyed")
        {
            mr.reset();

            THEN("the destructor is invoked")
            {
                REQUIRE(x == -1);
            }
        }

        WHEN("the managed resource is moved with move-construction")
        {
            auto new_mr = std::make_unique<ManagedResource<int*>>(
                std::move(*mr));

            THEN("ownership is moved to new object")
            {
                mr.reset();
                REQUIRE(x == 0);
                new_mr.reset();
                REQUIRE(x == -1);
            }
        }

        WHEN("the managed resource is moved with move-assignment")
        {
            int y = 0;
            auto new_mr = std::make_unique<ManagedResource<int*>>(
                std::move(&y), [] (auto const& p) { *p = -1; });

            *new_mr = std::move(*mr);

            THEN("old resource of new object is destroyed")
            {
                REQUIRE(y == -1);
            }

            THEN("ownership is moved to new object")
            {
                mr.reset();
                REQUIRE(x == 0);
                new_mr.reset();
                REQUIRE(x == -1);
            }
        }

        WHEN("the resource is stolen")
        {
            mr->steal();

            THEN("ownership is removed")
            {
                mr.reset();
                REQUIRE(x == 0);
            }
        }
    }
}
