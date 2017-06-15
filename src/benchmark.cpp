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

#include "benchmark.h"

#include "scene.h"
#include "log.h"
#include "util.h"

Benchmark::Benchmark(Scene& scene, std::vector<OptionPair> const& options)
    : scene_{scene}, options{options}
{
}

Scene& Benchmark::prepare_scene()
{
    scene_.reset_options();
    load_options();

    return scene_;
}

void Benchmark::load_options()
{
    for (auto const& option_pair : options)
    {
        if (!scene_.set_option(option_pair.first, option_pair.second))
        {
            auto const opt_iter = scene_.options().find(option_pair.first);

            if (opt_iter == scene_.options().end())
            {
                Log::info("Warning: Scene '%s' doesn't accept option '%s'\n",
                          scene_.name().c_str(),
                          option_pair.first.c_str());
            }
            else
            {
                Log::info("Warning: Scene '%s' doesn't accept value '%s' for option '%s'\n",
                          scene_.name().c_str(),
                          option_pair.second.c_str(),
                          option_pair.first.c_str());
            }
        }
    }
}
