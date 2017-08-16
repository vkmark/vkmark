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

#include "test_window_system_plugin.h"
#include "null_window_system.h"

#include "src/window_system_plugin.h"

namespace
{

class TestWindowSystem : public NullWindowSystem
{
public:
    TestWindowSystem(std::string const& id) : id{id} {}
    std::vector<char const*> vulkan_extensions() override { return {id.c_str()}; }

private:
    std::string const id;
};

int probe = 0;

}

void vkmark_window_system_load_options(Options&)
{
}

int vkmark_window_system_probe(Options const&)
{
    return probe;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const&)
{
    return std::make_unique<TestWindowSystem>(VKMARK_TEST_WINDOW_SYSTEM_ID);
}

void vkmark_test_window_system_configure_probe(int value)
{
    probe = value;
}
