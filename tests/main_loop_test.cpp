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

#include "src/main_loop.h"
#include "src/vulkan_image.h"
#include "src/scene_collection.h"
#include "src/benchmark_collection.h"
#include "src/options.h"

#include "test_scene.h"
#include "null_window_system.h"

#include "catch.hpp"

#include <string>
#include <thread>
#include <stdexcept>
#include <chrono>
#include <future>

using namespace Catch::Matchers;

namespace
{

std::string setup_log_entry(std::string const& name)
{
    return "(setup " + name + ")";
}

std::string start_log_entry(std::string const& name)
{
    return "(start " + name + ")";
}

std::string draw_log_entry(std::string const& name, uint32_t image_index)
{
    return "(draw " + name + " " + std::to_string(image_index) + ")";
}

std::string present_log_entry(uint32_t image_index)
{
    return "(present " + std::to_string(image_index) + ")";
}

std::vector<std::string> expected_log_for_scene_run(
    std::string const& name,
    uint32_t image_index)
{
    std::vector<std::string> expected;

    expected.push_back(setup_log_entry(name));
    expected.push_back(start_log_entry(name));
    expected.push_back(draw_log_entry(name, image_index));
    expected.push_back(present_log_entry(image_index));

    return expected;
}

class TestWindowSystem : public NullWindowSystem
{
public:
    TestWindowSystem(std::vector<std::string>& log)
        : log{log},
          should_quit_{false},
          image_index{0}
    {
    }

    VulkanImage next_vulkan_image() override
    {
        VulkanImage vi;
        vi.index = image_index++;
        return vi;
    }

    void present_vulkan_image(VulkanImage const& vi) override
    {
        log.push_back(present_log_entry(vi.index));
    }

    bool should_quit() override { return should_quit_; }

    void set_should_quit() { should_quit_ = true; }

private:
    std::vector<std::string>& log;
    std::atomic<bool> should_quit_;
    uint32_t image_index;
};

class SingleFrameScene : public TestScene
{
public:
    SingleFrameScene(
        std::string const& name,
        unsigned int target_fps,
        std::vector<std::string>& log)
        : TestScene{name}, target_fps{target_fps}, log{log}
    {
    }

    void setup(VulkanState& vulkan, std::vector<VulkanImage> const& images) override
    {
        log.push_back(setup_log_entry(name_));
        TestScene::setup(vulkan, images);
    }

    void start() override
    {
        log.push_back(start_log_entry(name_));
        TestScene::start();
    }

    void update() override
    {
        TestScene::update();
        running = false;
        // Set update time so that we get the target fps from
        // the Scene object, assuming 1 frame is rendered
        last_update_time = start_time + 1000000 / target_fps;
    }

    VulkanImage draw(VulkanImage const& vi) override
    {
        log.push_back(draw_log_entry(name_, vi.index));
        return TestScene::draw(vi);
    }

    static unsigned int fps(int i)
    {
        return i * i * 10;
    }

private:
    unsigned int target_fps;
    std::vector<std::string>& log;
};

}

SCENARIO("main loop run", "")
{
    std::vector<std::string> log;
    VulkanState* null_vulkan_state = nullptr;
    TestWindowSystem ws{log};

    SceneCollection sc;
    sc.register_scene(std::make_unique<SingleFrameScene>("", 0, log));
    sc.register_scene(
        std::make_unique<SingleFrameScene>(
            TestScene::name(1), SingleFrameScene::fps(1), log));
    sc.register_scene(
        std::make_unique<SingleFrameScene>(
            TestScene::name(2), SingleFrameScene::fps(2), log));
    sc.register_scene(
        std::make_unique<SingleFrameScene>(
            TestScene::name(3), SingleFrameScene::fps(3), log));

    BenchmarkCollection bc{sc};
    Options options;

    MainLoop main_loop{*null_vulkan_state, ws, bc, options};

    GIVEN("Some normal benchmarks")
    {
        std::vector<std::string> const benchmarks{
            TestScene::name(1), TestScene::name(2), TestScene::name(3)};
        bc.add(benchmarks);

        WHEN("running the main loop")
        {
            main_loop.run();

            THEN("the benchmarks are set up and run properly")
            {
                std::vector<std::string> expected;

                uint32_t i = 0;
                for (auto const& benchmark : benchmarks)
                {
                    auto const run_log = expected_log_for_scene_run(benchmark, i++);
                    expected.insert(expected.end(), run_log.begin(), run_log.end());
                }

                REQUIRE_THAT(log, Equals(expected));
            }

            THEN("the score is calculated as the average fps of the benchmarks")
            {
                auto const expected =
                    (SingleFrameScene::fps(1) +
                     SingleFrameScene::fps(2) +
                     SingleFrameScene::fps(3)) /
                    3;

                REQUIRE(main_loop.score() == expected);
            }
        }
    }

    GIVEN("Only option-setting benchmarks")
    {
        std::vector<std::string> const benchmarks{"", ":opt1=val1"};
        bc.add(benchmarks);

        WHEN("running the main loop")
        {
            main_loop.run();

            THEN("the benchmarks are only set up")
            {
                std::vector<std::string> const expected{
                    setup_log_entry(""),
                    setup_log_entry("")};

                REQUIRE_THAT(log, Equals(expected));
            }

            THEN("the score is zero")
            {
                REQUIRE(main_loop.score() == 0);
            }
        }
    }

    GIVEN("Some benchmarks including a few option-setting ones")
    {
        std::vector<std::string> const benchmarks{"", TestScene::name(1), ":opt1=val1"};
        bc.add(benchmarks);

        WHEN("running the main loop")
        {
            main_loop.run();

            THEN("the score is calculated based on non-option-setting benchmarks")
            {
                REQUIRE(main_loop.score() == SingleFrameScene::fps(1));
            }
        }
    }

    GIVEN("Some benchmarks including a few with invalid scene names")
    {
        std::vector<std::string> const benchmarks{
            "invalid1", TestScene::name(1), "invalid2", TestScene::name(2), "invalid3"};

        bc.add(benchmarks);

        WHEN("running the main loop")
        {
            main_loop.run();

            THEN("the benchmarks with invalid scene names are skipped")
            {
                std::vector<std::string> expected;

                uint32_t i = 0;
                for (auto const& benchmark : benchmarks)
                {
                    if (benchmark.find("invalid") == 0)
                        continue;
                    auto const run_log = expected_log_for_scene_run(benchmark, i++);
                    expected.insert(expected.end(), run_log.begin(), run_log.end());
                }

                REQUIRE_THAT(log, Equals(expected));
            }

            THEN("the score is calculated based on valid benchmarks only")
            {
                auto const expected =
                    (SingleFrameScene::fps(1) +
                     SingleFrameScene::fps(2)) /
                    2;

                REQUIRE(main_loop.score() == expected);
            }
        }
    }
}

SCENARIO("main loop stop", "")
{
    VulkanState* null_vulkan_state = nullptr;
    std::vector<std::string> log;
    TestWindowSystem ws{log};

    SceneCollection sc;
    sc.register_scene(std::make_unique<TestScene>(TestScene::name(1)));
    sc.register_scene(std::make_unique<TestScene>(TestScene::name(2)));
    sc.register_scene(std::make_unique<TestScene>(TestScene::name(3)));

    BenchmarkCollection bc{sc};
    Options options;

    MainLoop main_loop{*null_vulkan_state, ws, bc, options};

    GIVEN("Some benchmarks running")
    {
        std::vector<std::string> const benchmarks{
            TestScene::name(1), TestScene::name(2), TestScene::name(3)};
        bc.add(benchmarks);

        std::promise<void> done_promise;
        auto done_future = done_promise.get_future();
        std::chrono::seconds const done_timeout{3};

        auto loop_thread = std::thread{[&] { main_loop.run(); done_promise.set_value(); }};

        WHEN("stopping the main loop")
        {
            main_loop.stop();

            THEN("the loop exits")
            {
                if (done_future.wait_for(done_timeout) == std::future_status::ready)
                    loop_thread.join();
            }
        }

        WHEN("the window system wants to quit")
        {
            ws.set_should_quit();

            THEN("the loop exits")
            {
                if (done_future.wait_for(done_timeout) == std::future_status::ready)
                    loop_thread.join();
            }
        }
    }
}
