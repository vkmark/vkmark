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

#include <vector>

class VulkanState;
struct VulkanImage;

class WindowSystem
{
public:
    virtual ~WindowSystem() = default;

    virtual std::vector<char const*> vulkan_extensions() = 0;
    virtual void init_vulkan(VulkanState& vulkan) = 0;
    virtual void deinit_vulkan() = 0;

    virtual VulkanImage next_vulkan_image() = 0;
    virtual void present_vulkan_image(VulkanImage const&) = 0;

    virtual bool should_quit() = 0;

protected:
    WindowSystem() = default;
    WindowSystem(WindowSystem const&) = delete;
    WindowSystem& operator=(WindowSystem&) = delete;
};
