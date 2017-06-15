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

#include <memory>
#include <string>
#include <unordered_map>

class Scene;

class SceneCollection
{
public:
    SceneCollection();

    Scene& get_scene_by_name(std::string const& name);

    void set_option_default(std::string const& name, std::string const& value);
    void log_scene_info();

private:
    void register_scene(std::unique_ptr<Scene> scene);

    std::unique_ptr<Scene> dummy_scene;
    std::unordered_map<std::string,std::unique_ptr<Scene>> scene_map;
};
