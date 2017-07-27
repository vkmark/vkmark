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

#include "scene.h"
#include "vulkan_image.h"
#include "util.h"
#include "options.h"

SceneOption::SceneOption(std::string const& name,
                         std::string const& value,
                         std::string const& description,
                         std::string const& values)
    : name{name},
      value{value},
      default_value{value},
      description{description},
      acceptable_values{Util::split(values, ',')}
{

}

bool SceneOption::accepts_value(std::string const& val)
{
    return acceptable_values.empty() ||
           std::find(acceptable_values.begin(), acceptable_values.end(), val) !=
               acceptable_values.end();
}

Scene::Scene(std::string const& name)
    : name_{name},
      start_time{0}, last_update_time{0}, current_frame{0},
      running{false}, duration{0}
{
    options_["duration"] = SceneOption("duration", "10.0",
                                      "The duration of each benchmark in seconds");
}

bool Scene::is_valid() const
{
    return true;
}

void Scene::setup(VulkanState&, std::vector<VulkanImage> const&)
{
    duration = 1000000.0 * Util::from_string<double>(options_["duration"].value);
}

void Scene::teardown()
{
}

void Scene::start()
{
    current_frame = 0;
    running = true;
    start_time = Util::get_timestamp_us();
    last_update_time = start_time;
}

VulkanImage Scene::draw(VulkanImage const& image)
{
    return image.copy_with_semaphore({});
}

void Scene::update()
{
    auto const current_time = Util::get_timestamp_us();
    auto const elapsed_time = current_time - start_time;

    ++current_frame;

    last_update_time = current_time;

    if (elapsed_time >= duration)
        running = false;
}

std::string Scene::name() const
{
    return name_;
}

std::string Scene::info_string(bool show_all_options) const
{
    std::stringstream ss;

    ss << "[" << name_ << "] ";

    bool option_shown = false;

    for (auto const& kv : options_)
    {
        if (show_all_options || kv.second.set)
        {
            ss << kv.first << "=" << kv.second.value << ":";
            option_shown = true;
        }
    }

    if (!option_shown)
        ss << "<default>:";

    return ss.str();
}

unsigned int Scene::average_fps() const
{
    double const elapsed_time_sec = (last_update_time - start_time) / 1000000.0;
    return current_frame / elapsed_time_sec;
}

bool Scene::is_running() const
{
    return running;
}

bool Scene::set_option(std::string const& opt, std::string const& val)
{
    auto const iter = options_.find(opt);

    if (iter != options_.end() && iter->second.accepts_value(val))
    {
        iter->second.value = val;
        iter->second.set = true;
        return true;
    }

    return false;
}

void Scene::reset_options()
{
    for (auto& kv : options_)
    {
        auto& opt = kv.second;

        opt.value = opt.default_value;
        opt.set = false;
    }
}

bool Scene::set_option_default(std::string const& opt, std::string const& val)
{
    auto const iter = options_.find(opt);

    if (iter != options_.end() && iter->second.accepts_value(val))
    {
        iter->second.default_value = val;
        return true;
    }

    return false;
}

std::unordered_map<std::string, SceneOption> const& Scene::options() const
{
    return options_;
}
