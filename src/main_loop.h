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

#pragma once

#include <atomic>

class VulkanState;
class WindowSystem;
class BenchmarkCollection;
struct Options;

class MainLoop
{
public:
    MainLoop(
        VulkanState& vulkan,
        WindowSystem& ws,
        BenchmarkCollection& bc,
        Options const& options);

    void run();
    void stop();

    unsigned int score();

private:
    VulkanState& vulkan;
    WindowSystem& ws;
    BenchmarkCollection& bc;
    Options const& options;

    std::atomic<bool> should_stop;
    unsigned int total_fps;
    unsigned int total_benchmarks;
};
