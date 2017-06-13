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

#include "scene_collection.h"

#include "clear_scene.h"
#include "default_options_scene.h"
#include "log.h"

namespace
{

class DummyScene : public Scene
{
public:
    DummyScene(VulkanState& vulkan) : Scene{vulkan, "dummy"} {}
};

}

SceneCollection::SceneCollection(VulkanState& vulkan)
    : dummy_scene{std::make_unique<DummyScene>(vulkan)}
{
    register_scene(std::make_unique<ClearScene>(vulkan));
    register_scene(std::make_unique<DefaultOptionsScene>(vulkan, *this));
}

Scene& SceneCollection::get_scene_by_name(std::string const& name)
{
    auto const iter = scene_map.find(name);

    if (iter != scene_map.end())
        return *(iter->second);
    else
        return *dummy_scene;
}

void SceneCollection::register_scene(std::unique_ptr<Scene> scene)
{
    scene_map[scene->name()] = std::move(scene);
}

void SceneCollection::set_option_default(std::string const& name, std::string const& value)
{
    for (auto const& kv : scene_map)
    {
        Scene& scene = *kv.second;

        /* 
         * Display warning only if the option value is unsupported, not if
         * the scene doesn't support the option at all.
         */
        if (!scene.set_option_default(name, value) &&
            scene.options().find(name) != scene.options().end())
        {
            Log::info("Warning: Scene '%s' doesn't accept default value '%s' for option '%s'\n",
                      scene.name().c_str(), value.c_str(), name.c_str());
        }
    }
}

void SceneCollection::log_scene_info()
{
    for (auto const& kv : scene_map)
    {
        auto const scene = kv.second.get();
        if (scene->name().empty())
            continue;

        Log::info("[Scene] %s\n", scene->name().c_str());

        for (auto const& opt_kv : scene->options())
        {
            auto const& opt = opt_kv.second;

            Log::info("  [Option] %s\n"
                      "    Description  : %s\n"
                      "    Default Value: %s\n",
                      opt.name.c_str(),
                      opt.description.c_str(),
                      opt.default_value.c_str());

            /* Display list of acceptable values (if defined) */
            if (!opt.acceptable_values.empty()) {
                Log::info("    Acceptable Values: ");

                for (auto val_iter = opt.acceptable_values.cbegin();
                     val_iter != opt.acceptable_values.cend();
                     ++val_iter)
                {
                    std::string format_value{Log::continuation_prefix + "%s"};
                    if (val_iter + 1 != opt.acceptable_values.end())
                        format_value += ",";
                    else
                        format_value += "\n";
                    Log::info(format_value.c_str(), val_iter->c_str());
                }
            }
        }
    }
}
