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
#include "scene.h"
#include "scene_collection.h"
#include "benchmark_collection.h"
#include "default_benchmarks.h"
#include "options.h"
#include "log.h"
#include "util.h"
#include "main_loop.h"

#include "scenes/clear_scene.h"
#include "scenes/cube_scene.h"
#include "scenes/default_options_scene.h"
#include "scenes/desktop_scene.h"
#include "scenes/effect2d_scene.h"
#include "scenes/shading_scene.h"
#include "scenes/texture_scene.h"
#include "scenes/vertex_scene.h"

#include <stdexcept>
#include <csignal>
#include <memory>
#include <iostream>

namespace
{

MainLoop* main_loop_global = nullptr;

void sighandler(int)
{
    main_loop_global->stop();
}

void set_up_sighandler(MainLoop& main_loop)
{
    main_loop_global = &main_loop;

    struct sigaction sa{};
    sa.sa_handler = sighandler;

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
}

void populate_scene_collection(SceneCollection& sc)
{
    sc.register_scene(std::make_unique<ClearScene>());
    sc.register_scene(std::make_unique<CubeScene>());
    sc.register_scene(std::make_unique<DefaultOptionsScene>(sc));
    sc.register_scene(std::make_unique<DesktopScene>());
    sc.register_scene(std::make_unique<Effect2DScene>());
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

    auto vulkan = options.use_device_with_uuid.second ?
        VulkanState{ws.vulkan_wsi(), ChooseByUUIDStrategy{options.use_device_with_uuid.first}} :
        VulkanState{ws.vulkan_wsi(), ChooseFirstSupportedStrategy{}};

    if (options.list_devices)
    {
        log_info(vulkan.instance().enumeratePhysicalDevices());
        return 0;
    }

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

    MainLoop main_loop{vulkan, ws, bc, options};

    set_up_sighandler(main_loop);

    main_loop.run();

    Log::info("=======================================================\n");
    Log::info("                                   vkmark Score: %u\n",
              main_loop.score());
    Log::info("=======================================================\n");

}
catch (std::exception const& e)
{
    Log::error("%s\n", e.what());
    return 1;
}
