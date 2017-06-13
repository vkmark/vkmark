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

#pragma once

#include <vector>
#include <string>
#include <memory>

class Benchmark;
class SceneCollection;

class BenchmarkCollection
{
public:
    BenchmarkCollection(SceneCollection& scene_collection);

    void add(std::vector<std::string> const& benchmarks);
    std::vector<Benchmark*> benchmarks() const;

private:
    SceneCollection& scene_collection;
    std::vector<std::unique_ptr<Benchmark>> benchmarks_;
};
