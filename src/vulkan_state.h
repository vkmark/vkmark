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

#include <vulkan/vulkan.hpp>

#include "managed_resource.h"

class VulkanState
{
public:
    VulkanState(std::vector<char const*> required_extensions);

    void log_info();
    void set_surface(vk::SurfaceKHR surface);

    vk::Instance const& instance() const
    {
        return vk_instance;
    }

    vk::PhysicalDevice const& physical_device() const
    {
        return vk_physical_device;
    }

    uint32_t const& graphics_queue_family_index() const
    {
        return vk_graphics_queue_family_index;
    }

    vk::Device const& device() const
    {
        return vk_device;
    }

    vk::Queue const& graphics_queue() const
    {
        return vk_graphics_queue;
    }

    vk::CommandPool const& command_pool() const
    {
        return vk_command_pool;
    }


private:
    void create_instance(std::vector<char const*> const& required_extensions);
    void choose_physical_device();
    void create_device();
    void create_command_pool();

    ManagedResource<vk::Instance> vk_instance;
    vk::PhysicalDevice vk_physical_device;
    uint32_t vk_graphics_queue_family_index;
    ManagedResource<vk::Device> vk_device;
    vk::Queue vk_graphics_queue;
    ManagedResource<vk::CommandPool> vk_command_pool;
};
