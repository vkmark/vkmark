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

#include "main_loop.h"
#include "vulkan_state.h"
#include "vulkan_image.h"
#include "window_system.h"
#include "benchmark_collection.h"
#include "benchmark.h"
#include "scene.h"
#include "log.h"
#include "options.h"
#include "util.h"

namespace
{

void log_scene_info(Scene& scene, bool show_all_options)
{
    Log::info("%s\n", scene.info_string(show_all_options).c_str());
    Log::flush();
}

void log_scene_invalid(Scene& scene)
{
    Log::warning("Skipping benchmark with invalid scene name '%s'\n",
                 scene.name().c_str());
    Log::flush();
}

void log_scene_exception(std::string const& what)
{
    auto const fmt = Log::continuation_prefix + " Failed with exception: %s\n";
    Log::info(fmt.c_str(), what.c_str());
    Log::flush();
}

void log_scene_fps(unsigned int fps)
{
    auto const fmt = Log::continuation_prefix + " FPS: %u FrameTime: %.3f ms\n";
    Log::info(fmt.c_str(), fps, 1000.0 / fps);
    Log::flush();
}


template <typename T>
void advance_iter(T& iter, T const& start, T const& end, bool run_forever)
{
    ++iter;
    if (run_forever && iter == end)
        iter = start;
}

}

MainLoop::MainLoop(VulkanState& vulkan,
                   WindowSystem& ws,
                   BenchmarkCollection& bc,
                   Options const& options)
    : vulkan{vulkan}, ws{ws}, bc{bc}, options{options},
      should_stop{false},
      total_fps{0},
      total_benchmarks{0}
{
}

void MainLoop::run()
{
    auto const& benchmarks = bc.benchmarks();

    for (auto iter = benchmarks.begin();
         iter != benchmarks.end();
         advance_iter(iter, benchmarks.begin(), benchmarks.end(), options.run_forever))
    try
    {
        auto& benchmark = *iter;

        auto& scene = benchmark->prepare_scene();

        if (!scene.is_valid())
        {
            log_scene_invalid(scene);
            continue;
        }

        // Scenes with empty names are option-setting scenes.
        // Just set them up and continue.
        if (scene.name().empty())
        {
            scene.setup(vulkan, ws.vulkan_images());
            continue;
        }

        log_scene_info(scene, options.show_all_options);

        auto const scene_teardown = Util::on_scope_exit([&] { scene.teardown(); });
        scene.setup(vulkan, ws.vulkan_images());

        bool should_quit = false;

        scene.start();

        while (scene.is_running() &&
               !(should_quit = ws.should_quit()) &&
               !should_stop)
        {
            ws.present_vulkan_image(
                scene.draw(ws.next_vulkan_image()));
            scene.update();
        }

        auto const scene_fps = scene.average_fps();

        log_scene_fps(scene_fps);

        total_fps += scene_fps;
        ++total_benchmarks;

        if (should_quit || should_stop)
            break;
    }
    catch (std::exception const& e)
    {
        log_scene_exception(e.what());
    }
}

void MainLoop::stop()
{
    should_stop = true;
}

unsigned int MainLoop::score()
{
    return total_benchmarks == 0 ? 0 : (total_fps / total_benchmarks);
}
