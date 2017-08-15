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

#include "src/benchmark_collection.h"
#include "src/scene_collection.h"
#include "src/scene.h"
#include "src/benchmark.h"

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

std::string benchmark_string(
    std::string const& name,
    std::string const& value1 = "",
    std::string const& value2 = "",
    std::string const& extra = "")
{
    auto ret = name;

    if (!value1.empty())
        ret += ":" + option1.name + "=" + value1;

    if (!value2.empty())
        ret += ":" + option2.name + "=" + value2;

    ret += extra;

    return ret;
}

}

SCENARIO("benchmark collection", "")
{
    SceneCollection sc;
    BenchmarkCollection bc{sc};

    GIVEN("A few registered scenes")
    {
        auto const option_setting_scene_name = "";
        auto const scene_name_1 = TestScene::name(1);
        auto const scene_name_2 = TestScene::name(2);
        auto const scene_name_3 = TestScene::name(3);

        sc.register_scene(std::make_unique<TestScene>(option_setting_scene_name));
        sc.register_scene(std::make_unique<TestSceneWithOptions>(scene_name_1));
        sc.register_scene(std::make_unique<TestScene>(scene_name_2));
        sc.register_scene(std::make_unique<TestSceneWithOptions>(scene_name_3));

        WHEN("adding a benchmark for an unregistered scene")
        {
            bc.add({"unregistered"});

            THEN("an invalid benchmark is returned")
            {
                auto const benchmarks = bc.benchmarks();
                REQUIRE(benchmarks.size() == 1);
                REQUIRE_FALSE(benchmarks[0]->prepare_scene().is_valid());
            }
        }

        WHEN("adding a benchmark for a registered scene")
        {
            bc.add({scene_name_1});

            THEN("the benchmark is returned")
            {
                auto const benchmarks = bc.benchmarks();
                REQUIRE(benchmarks.size() == 1);
                REQUIRE(benchmarks[0]->prepare_scene().name() == scene_name_1);
            }
        }

        WHEN("adding a benchmark for a registered scene with options")
        {
            bc.add({benchmark_string(scene_name_1, "1", "2")});

            THEN("the benchmark with set options is returned")
            {
                auto const benchmarks = bc.benchmarks();
                auto const& scene = benchmarks.at(0)->prepare_scene();

                REQUIRE(benchmarks.size() == 1);
                REQUIRE(scene.name() == scene_name_1);
                REQUIRE(scene.options().at(option1.name).value == "1");
                REQUIRE(scene.options().at(option2.name).value == "2");
            }
        }

        WHEN("adding a benchmark with invalid options")
        {
            bc.add({benchmark_string(scene_name_1, "", "2",
                                     ":testoptinvalid=1:testoptinvalid1=bla")});

            THEN("the invalid options are ignored")
            {
                auto const benchmarks = bc.benchmarks();
                auto const& scene = benchmarks.at(0)->prepare_scene();

                REQUIRE(benchmarks.size() == 1);
                REQUIRE(scene.name() == scene_name_1);
                REQUIRE(scene.options().at(option1.name).value == option1.value);
                REQUIRE(scene.options().at(option2.name).value == "2");
            }
        }
        WHEN("adding a benchmark for many registered scene with options")
        {
            bc.add({
                benchmark_string(scene_name_1, "1", "2"),
                benchmark_string(scene_name_2),
                benchmark_string(scene_name_1, "3", "4"),
                });

            THEN("all benchmarks with set options are returned")
            {
                auto const benchmarks = bc.benchmarks();
                REQUIRE(benchmarks.size() == 3);

                auto const& scene1 = benchmarks.at(0)->prepare_scene();
                REQUIRE(scene1.name() == scene_name_1);
                REQUIRE(scene1.options().at(option1.name).value == "1");
                REQUIRE(scene1.options().at(option2.name).value == "2");

                auto const& scene2 = benchmarks.at(1)->prepare_scene();
                REQUIRE(scene2.name() == scene_name_2);
                REQUIRE_THROWS(scene2.options().at(option1.name));
                REQUIRE_THROWS(scene2.options().at(option2.name));

                auto const& scene3 = benchmarks.at(2)->prepare_scene();
                REQUIRE(scene3.name() == scene_name_1);
                REQUIRE(scene3.options().at(option1.name).value == "3");
                REQUIRE(scene3.options().at(option2.name).value == "4");
            }
        }
        WHEN("adding a benchmark for an option-setting scene")
        {
            bc.add({benchmark_string(option_setting_scene_name)});

            THEN("the option-setting benchmark is returned")
            {
                auto const benchmarks = bc.benchmarks();
                REQUIRE(benchmarks.size() == 1);

                auto const& scene1 = benchmarks.at(0)->prepare_scene();
                REQUIRE(scene1.name() == option_setting_scene_name);
            }
        }
    }
}

SCENARIO("benchmark collection contains normal scenes", "")
{
    SceneCollection sc;
    BenchmarkCollection bc{sc};

    GIVEN("A few registered scenes including an option-setting one")
    {
        auto const scene_name_1 = TestScene::name(1);
        auto const option_setting_scene_name = "";

        sc.register_scene(std::make_unique<TestScene>(scene_name_1));
        sc.register_scene(std::make_unique<TestScene>(option_setting_scene_name));

        WHEN("the benchmark collection is empty")
        {
            THEN("contains no normal scenes is reported")
            {
                REQUIRE_FALSE(bc.contains_normal_scenes());
            }
        }

        WHEN("adding only option-setting scenes")
        {
            bc.add({":duration=1", ":bla=2"});

            THEN("contains no normal scenes is reported")
            {
                REQUIRE_FALSE(bc.contains_normal_scenes());
            }
        }

        WHEN("adding at least one normal scene")
        {
            bc.add({":duration=1", scene_name_1});

            THEN("contains normal scenes is reported")
            {
                REQUIRE(bc.contains_normal_scenes());
            }
        }
    }
}
