/*
 * Copyright © 2017 Collabora Ltd.
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vkmark.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

#include "window_system.h"
#include "window_system_loader.h"
#include "vulkan_state.h"
#include "vulkan_image.h"
#include "scene.h"
#include "scene_collection.h"
#include "benchmark.h"
#include "benchmark_collection.h"
#include "default_benchmarks.h"
#include "options.h"
#include "log.h"

#include <stdexcept>

namespace
{

void log_scene_info(Scene& scene, bool show_all_options)
{
    Log::info("%s", scene.info_string(show_all_options).c_str());
    Log::flush();
}

void log_scene_setup_failure()
{
    auto const fmt = Log::continuation_prefix + " Setup failed";
    Log::info(fmt.c_str());
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

struct WindowSystemVulkanInit
{
    WindowSystemVulkanInit(WindowSystem& ws, VulkanState& vulkan) : ws{ws}
    {
        ws.init_vulkan(vulkan);
    }
    ~WindowSystemVulkanInit()
    {
        ws.deinit_vulkan();
    }
    WindowSystem& ws;
};

}

int main(int argc, char **argv)
try
{
    Options options;

    if (!options.parse_args(argc, argv))
        return 1;

    Log::init(argv[0], options.show_debug);

    if (options.show_help)
    {
        options.print_help();
        return 0;
    }

    WindowSystemLoader ws_loader{options};
    auto& ws = ws_loader.load_window_system();

    VulkanState vulkan{ws.vulkan_extensions()};

    WindowSystemVulkanInit ws_vulkan_init{ws, vulkan};

    Log::info("=======================================================\n");
    Log::info("    vkmark %s\n", VKMARK_VERSION_STR);
    Log::info("=======================================================\n");
    vulkan.log_info();
    Log::info("=======================================================\n");

    SceneCollection sc{vulkan};
    BenchmarkCollection bc{sc};

    if (options.list_scenes)
    {
        sc.log_scene_info();
        return 0;
    }

    if (!options.benchmarks.empty())
        bc.add(options.benchmarks);
    else
        bc.add(DefaultBenchmarks::get());

    unsigned int total_fps = 0;
    unsigned int total_benchmarks = 0;

    for (auto const& benchmark : bc.benchmarks())
    try
    {
        auto& scene = benchmark->setup_scene();

        if (scene.name().empty())
            continue;

        log_scene_info(scene, options.show_all_options);

        if (!scene.is_running())
        {
            log_scene_setup_failure();
            continue;
        }

        bool should_quit = false;

        while (scene.is_running() && !(should_quit = ws.should_quit()))
        {
            ws.present_vulkan_image(
                scene.draw(ws.next_vulkan_image()));
            scene.update();
        }
        
        auto const scene_fps = scene.average_fps();

        log_scene_fps(scene_fps);

        total_fps += scene_fps;
        ++total_benchmarks;

        benchmark->teardown_scene();

        if (should_quit)
            break;
    }
    catch (std::exception const& e)
    {
        log_scene_exception(e.what());
    }

    Log::info("=======================================================\n");
    Log::info("                                   vkmark Score: %u\n",
              total_benchmarks == 0 ? 0 : total_fps / total_benchmarks);
    Log::info("=======================================================\n");
}
catch (std::exception const& e)
{
    Log::error("%s\n", e.what());
    return 1;
}
