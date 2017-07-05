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

#include "test_scene.h"

TestScene::TestScene(std::string const& name)
    : TestScene{name, {}}
{
}

TestScene::TestScene(
    std::string const& name,
    std::vector<SceneOption> const& options)
    : Scene{name}
{
    for (auto const& opt : options)
        options_[opt.name] = opt;
}

std::string TestScene::name(int i)
{
    return "test_scene_" + std::to_string(i);
}
