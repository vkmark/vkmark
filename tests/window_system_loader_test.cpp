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

#include "src/window_system_loader.h"
#include "src/window_system.h"
#include "src/options.h"
#include "src/vulkan_wsi.h"

#include "test_window_system_plugin.h"

#include "catch.hpp"

#include <dlfcn.h>

using namespace Catch::Matchers;

namespace
{

class TestWindowSystems
{
public:
    TestWindowSystems()
        : handles{{open_window_system(1),
                   open_window_system(2),
                   open_window_system(3)}}
    {
    }

    void configure_probe(int id, int probe)
    {
        auto const handle = handles.at(id - 1).get();
        auto const configure_probe =
            reinterpret_cast<VkMarkTestWindowSystemConfigureProbeFunc>(
                dlsym(handle, "vkmark_test_window_system_configure_probe"));

        if (!configure_probe)
            throw std::runtime_error{"Failed to find configure probe function"};

        configure_probe(probe);
    }

    static std::string name_from_id(int id)
    {
        return "testws" + std::to_string(id);
    }

private:
    using LibHandle = std::unique_ptr<void, int(*)(void*)>;

    std::array<LibHandle, 3> handles;

    LibHandle open_window_system(int id)
    {
        auto const path = std::string{VKMARK_TEST_WINDOW_SYSTEM_DIR} + "/" +
                          name_from_id(id) + ".so";
        auto const handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle)
            throw std::runtime_error{"Failed to load test window system " + path};

        return {handle, dlclose};
    }

};

int window_system_id(WindowSystem& ws)
{
    return std::stoi(ws.vulkan_wsi().required_extensions().instance[0]);
}

}

SCENARIO("window system loader", "")
{
    Options options;
    options.window_system_dir = VKMARK_TEST_WINDOW_SYSTEM_DIR;

    WindowSystemLoader wsl{options};
    TestWindowSystems tws;

    GIVEN("Window systems that don't match")
    {
        tws.configure_probe(1, 0);
        tws.configure_probe(2, 0);
        tws.configure_probe(3, 0);

        WHEN("loading a window system")
        {
            THEN("no window system is loaded")
            {
                REQUIRE_THROWS(wsl.load_window_system());
            }
        }
    }

    GIVEN("A window system that is the best match")
    {
        tws.configure_probe(1, 127);
        tws.configure_probe(2, 255);
        tws.configure_probe(3, 0);

        WHEN("loading a window system")
        {
            auto& ws = wsl.load_window_system();

            THEN("the best window system is loaded")
            {
                REQUIRE(window_system_id(ws) == 2);
            }
        }
    }

    GIVEN("Multiple window systems that are best matches")
    {
        tws.configure_probe(1, 127);
        tws.configure_probe(2, 255);
        tws.configure_probe(3, 255);

        WHEN("loading a window system")
        {
            auto& ws = wsl.load_window_system();

            THEN("any of the best matches is loaded")
            {
                REQUIRE((window_system_id(ws) == 2 ||
                         window_system_id(ws) == 3));
            }
        }
    }
}

SCENARIO("window system loader with user preference", "")
{
    Options options;
    options.window_system_dir = VKMARK_TEST_WINDOW_SYSTEM_DIR;

    TestWindowSystems tws;
    tws.configure_probe(1, 127);
    tws.configure_probe(2, 255);
    tws.configure_probe(3, 0);

    GIVEN("A user prefers an existing usable window system")
    {
        options.window_system = tws.name_from_id(1);

        WHEN("loading a window system")
        {
            WindowSystemLoader wsl{options};
            auto& ws = wsl.load_window_system();

            THEN("the user preferred one is loaded")
            {
                REQUIRE(window_system_id(ws) == 1);
            }
        }
    }

    GIVEN("A user prefers an existing unusable window system")
    {
        options.window_system = tws.name_from_id(3);

        WHEN("loading a window system")
        {
            WindowSystemLoader wsl{options};
            auto& ws = wsl.load_window_system();

            THEN("the user preferred one is loaded")
            {
                REQUIRE(window_system_id(ws) == 3);
            }
        }
    }

    GIVEN("A user prefers a non-existent window system")
    {
        options.window_system = "invalid";

        WHEN("loading a window system")
        {
            WindowSystemLoader wsl{options};

            THEN("no window system is loaded")
            {
                REQUIRE_THROWS(wsl.load_window_system());
            }
        }
    }
}
