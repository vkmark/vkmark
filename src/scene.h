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

#include <string>
#include <vector>
#include <unordered_map>

class SceneOption
{
public:
    SceneOption(std::string const& name,
                std::string const& value,
                std::string const& desc,
                std::string const& values = "");
    SceneOption() = default;

    bool accepts_value(std::string const& val);

    std::string name;
    std::string value;
    std::string default_value;
    std::string description;
    std::vector<std::string> acceptable_values;
    bool set;
};

class VulkanState;
struct VulkanImage;

class Scene
{
public:
    virtual ~Scene() = default;

    virtual bool is_valid() const;
    virtual void setup(VulkanState&, std::vector<VulkanImage> const&);
    virtual void teardown();

    virtual void start();
    virtual VulkanImage draw(VulkanImage const&);
    virtual void update();

    std::string name() const;
    std::string info_string(bool show_all_options) const;
    unsigned int average_fps() const;
    bool is_running() const;

    bool set_option(std::string const& opt, std::string const& val);
    void reset_options();
    bool set_option_default(std::string const& opt, std::string const& val);
    std::unordered_map<std::string, SceneOption> const& options() const;

protected:
    Scene(std::string const& name);

    std::string const name_;
    std::unordered_map<std::string,SceneOption> options_;
    uint64_t start_time;
    uint64_t last_update_time;
    uint64_t current_frame;
    bool running;
    uint64_t duration;
};
