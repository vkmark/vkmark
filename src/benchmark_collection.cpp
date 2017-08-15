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

#include "benchmark_collection.h"

#include "benchmark.h"
#include "scene_collection.h"
#include "scene.h"
#include "log.h"
#include "util.h"

namespace
{

std::string get_name_from_description(std::string const& s)
{
    auto const elems = Util::split(s, ':');

    return !elems.empty() ? elems[0] : "";
}

std::vector<Benchmark::OptionPair> get_options_from_description(std::string const& s)
{
    std::vector<Benchmark::OptionPair> options;

    auto const elems = Util::split(s, ':');

    if (elems.empty())
        return options;

    for (auto iter = elems.begin() + 1;
         iter != elems.end();
         ++iter)
    {
        auto const opt = Util::split(*iter, '=');
        if (opt.size() == 2)
            options.emplace_back(opt[0], opt[1]);
        else
            Log::info("Warning: ignoring invalid option string '%s' "
                      "in benchmark description\n",
                      iter->c_str());
    }

    return options;
}

}

BenchmarkCollection::BenchmarkCollection(SceneCollection& scene_collection)
    : scene_collection{scene_collection},
      contains_normal_scenes_{false}
{
}

void BenchmarkCollection::add(std::vector<std::string> const& benchmark_strings)
{
    for (auto const& bstr : benchmark_strings)
    {
        auto const scene_name = get_name_from_description(bstr);
        auto const options = get_options_from_description(bstr);
        auto& scene = scene_collection.get_scene_by_name(scene_name);

        if (!scene.name().empty())
            contains_normal_scenes_ = true;

        benchmarks_.push_back(std::make_unique<Benchmark>(scene, options));
    }
}

std::vector<Benchmark*> BenchmarkCollection::benchmarks() const
{ 
    std::vector<Benchmark*> benchmarks_raw;

    for (auto const& b : benchmarks_)
        benchmarks_raw.push_back(b.get());

    return benchmarks_raw;
}

bool BenchmarkCollection::contains_normal_scenes() const
{
    return contains_normal_scenes_;
}
