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

#include "default_options_scene.h"

#include "scene_collection.h"

DefaultOptionsScene::DefaultOptionsScene(SceneCollection& scene_collection)
    : Scene{""},
      scene_collection{scene_collection}
{
}

bool DefaultOptionsScene::setup(VulkanState&, std::vector<VulkanImage> const&)
{
    for (auto const& kv : options_)
    {
        if (kv.second.set)
            scene_collection.set_option_default(kv.first, kv.second.value);
    }

    return true;
}
