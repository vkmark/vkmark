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
#include "util.h"

#include "scenes/clear_scene.h"
#include "scenes/cube_scene.h"
#include "scenes/default_options_scene.h"
#include "scenes/shading_scene.h"
#include "scenes/texture_scene.h"
#include "scenes/vertex_scene.h"

#include <stdexcept>
#include <csignal>
#include <atomic>
#include <memory>
#include <iostream>

namespace
{

std::atomic<bool> should_quit_sig{false};

void sighandler(int)
{
    should_quit_sig = true;
}

void set_up_sighandler()
{
    struct sigaction sa{};
    sa.sa_handler = sighandler;

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
}

void log_scene_info(Scene& scene, bool show_all_options)
{
    Log::info("%s", scene.info_string(show_all_options).c_str());
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

void populate_scene_collection(SceneCollection& sc)
{
    sc.register_scene(std::make_unique<ClearScene>());
    sc.register_scene(std::make_unique<CubeScene>());
    sc.register_scene(std::make_unique<DefaultOptionsScene>(sc));
    sc.register_scene(std::make_unique<ShadingScene>());
    sc.register_scene(std::make_unique<TextureScene>());
    sc.register_scene(std::make_unique<VertexScene>());
}

}

int main(int argc, char **argv)
try
{
    Options options;

    if (!options.parse_args(argc, argv))
        return 1;

    Log::init(argv[0], options.show_debug);

    WindowSystemLoader ws_loader{options};
    ws_loader.load_window_system_options();

    if (options.show_help)
    {
        std::cout << options.help_string();
        return 0;
    }

    Util::set_data_dir(options.data_dir);

    SceneCollection sc;
    populate_scene_collection(sc);

    BenchmarkCollection bc{sc};

    if (options.list_scenes)
    {
        sc.log_scene_info();
        return 0;
    }

    auto& ws = ws_loader.load_window_system();
    VulkanState vulkan{ws.vulkan_extensions()};

    auto const ws_vulkan_deinit = Util::on_scope_exit([&] { ws.deinit_vulkan(); });
    ws.init_vulkan(vulkan);

    Log::info("=======================================================\n");
    Log::info("    vkmark %s\n", VKMARK_VERSION_STR);
    Log::info("=======================================================\n");
    vulkan.log_info();
    Log::info("=======================================================\n");

    if (!options.benchmarks.empty())
        bc.add(options.benchmarks);

    if (!bc.contains_normal_scenes())
        bc.add(DefaultBenchmarks::get());

    set_up_sighandler();

    unsigned int total_fps = 0;
    unsigned int total_benchmarks = 0;

    for (auto const& benchmark : bc.benchmarks())
    try
    {
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
               !should_quit_sig)
        {
            ws.present_vulkan_image(
                scene.draw(ws.next_vulkan_image()));
            scene.update();
        }
        
        auto const scene_fps = scene.average_fps();

        log_scene_fps(scene_fps);

        total_fps += scene_fps;
        ++total_benchmarks;

        if (should_quit || should_quit_sig)
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
