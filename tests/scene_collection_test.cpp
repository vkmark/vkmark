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

#include "src/scene_collection.h"
#include "src/scene.h"

#include "test_scene.h"
#include "catch.hpp"

using namespace Catch::Matchers;

namespace
{

SceneOption const option1{"option1", "value1", ""};
SceneOption const option2{"option2", "value2", ""};

struct TestSceneWithOptions : TestScene
{
    TestSceneWithOptions(std::string const& name)
        : TestScene{name, {option1, option2}}
    {
    }
};

}

SCENARIO("scene collection", "")
{
    SceneCollection sc;

    GIVEN("A few registered scenes")
    {
        auto const scene_name_1 = TestScene::name(1);
        auto const scene_name_2 = TestScene::name(2);
        auto const scene_name_3 = TestScene::name(3);

        sc.register_scene(std::make_unique<TestSceneWithOptions>(scene_name_1));
        sc.register_scene(std::make_unique<TestScene>(scene_name_2));
        sc.register_scene(std::make_unique<TestSceneWithOptions>(scene_name_3));

        WHEN("searching for a registered scene by name")
        {
            auto const& scene = sc.get_scene_by_name(scene_name_1);

            THEN("the scene is returned")
            {
                REQUIRE(scene.name() == scene_name_1);
                REQUIRE(scene.is_valid());
            }
        }

        WHEN("searching for an unregistered scene by name")
        {
            auto const& scene = sc.get_scene_by_name("unregistered");

            THEN("an invalid scene is returned")
            {
                REQUIRE(scene.name() == "unregistered");
                REQUIRE_FALSE(scene.is_valid());
            }
        }

        WHEN("setting an option default value")
        {
            std::string const new_opt_value{"newtestval"};
            sc.set_option_default(option1.name, new_opt_value);

            THEN("all registered scenes with the option are updated")
            {
                auto const& scene1 = sc.get_scene_by_name(scene_name_1);
                auto const& scene3 = sc.get_scene_by_name(scene_name_3);

                REQUIRE(scene1.options().at(option1.name).default_value == new_opt_value);
                REQUIRE(scene3.options().at(option1.name).default_value == new_opt_value);
            }

            THEN("all registered scenes without the option are not affected")
            {
                auto const& scene2 = sc.get_scene_by_name(scene_name_2);
                REQUIRE_THROWS(scene2.options().at(option1.name).default_value == new_opt_value);
            }

            THEN("other options are not affected")
            {
                auto const& scene1 = sc.get_scene_by_name(scene_name_1);
                auto const& scene3 = sc.get_scene_by_name(scene_name_3);

                REQUIRE(scene1.options().at(option2.name).default_value == option2.default_value);
                REQUIRE(scene3.options().at(option2.name).default_value == option2.default_value);
            }
        }
    }
}
