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
#include "vulkan_wsi.h"


void log_info(vk::PhysicalDevice const& physical_device);
void log_info(std::vector<vk::PhysicalDevice> const& physical_devices);

class VulkanState
{
public:
    template<typename ChoosePhysicalDeviceStrategy>
    VulkanState(VulkanWSI& vulkan_wsi, ChoosePhysicalDeviceStrategy pd_strategy)
    {
        create_instance(vulkan_wsi);
        create_physical_device(vulkan_wsi, pd_strategy);
        create_logical_device(vulkan_wsi);
        create_command_pool();
    }

    template<typename ChoosePhysicalDeviceStrategy>
    void create_physical_device(VulkanWSI& vulkan_wsi, ChoosePhysicalDeviceStrategy pd_strategy)
    {
        vk_physical_device = pd_strategy(avaiable_devices(vulkan_wsi));
    }

    void log_info()
    {
        ::log_info(physical_device());
    }

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
    void create_instance(VulkanWSI& vulkan_wsi);
    void choose_physical_device(VulkanWSI& vulkan_wsi);
    void create_logical_device(VulkanWSI& vulkan_wsi);
    void create_command_pool();
    std::vector<vk::PhysicalDevice> avaiable_devices(VulkanWSI& vulkan_wsi);

    ManagedResource<vk::Instance> vk_instance;
    ManagedResource<vk::Device> vk_device;
    ManagedResource<vk::CommandPool> vk_command_pool;
    vk::Queue vk_graphics_queue;
    vk::PhysicalDevice vk_physical_device;
    uint32_t vk_graphics_queue_family_index;
    
};

#include "device_uuid.h"

// template strategies seems to require simpler code than polymorphic
class ChooseFirstSupportedStrategy
{
public:
    vk::PhysicalDevice operator()(const std::vector<vk::PhysicalDevice>& avaiable_devices);
};

class ChooseByUUIDStrategy
{
public:
    ChooseByUUIDStrategy(const DeviceUUID& uuid)
        : m_selected_device_uuid(uuid)
    {}

    vk::PhysicalDevice operator()(const std::vector<vk::PhysicalDevice>& avaiable_devices);

private:
    DeviceUUID m_selected_device_uuid;
};