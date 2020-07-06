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
#include <optional>

#include "managed_resource.h"

class VulkanWSI;
class ChoosePhysicalDeviceStrategy;
void log_info(vk::PhysicalDevice const& physical_device);
void log_info(std::vector<vk::PhysicalDevice> const& physical_devices);

class VulkanState
{
public:
    VulkanState(VulkanWSI& vulkan_wsi, ChoosePhysicalDeviceStrategy& choose_physical_device_strategy);

    inline void log_info()
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
    void create_device(VulkanWSI& vulkan_wsi);
    void create_command_pool();

    ManagedResource<vk::Instance> vk_instance;
    ManagedResource<vk::Device> vk_device;
    vk::Queue vk_graphics_queue;
    ManagedResource<vk::CommandPool> vk_command_pool;
    std::pair<size_t, bool> force_physical_device; // pseudo-optional
    vk::PhysicalDevice vk_physical_device;
    uint32_t vk_graphics_queue_family_index;
    
};

class ChoosePhysicalDeviceStrategy
{
public:
    virtual ~ChoosePhysicalDeviceStrategy(){};

    inline vk::PhysicalDevice const& physical_device() const noexcept
    {
        return vk_physical_device;
    }

    inline uint32_t const& graphics_queue_family_index() const noexcept
    {
        return vk_graphics_queue_family_index;
    }

    virtual void choose(vk::Instance const& vk_instance, VulkanWSI& vulkan_wsi) = 0;

protected:
    vk::PhysicalDevice vk_physical_device;
    uint32_t vk_graphics_queue_family_index;
};

class ChooseFirstSupportedStrategy : public ChoosePhysicalDeviceStrategy
{
public:
    void choose(vk::Instance const& vk_instance, VulkanWSI& vulkan_wsi) override;
};

class ChooseByIndexStrategy : public ChoosePhysicalDeviceStrategy
{
public:
    inline ChooseByIndexStrategy(uint32_t use_physical_device_with_index)
    {
        physical_device_index = use_physical_device_with_index;
    }

    void choose(vk::Instance const& vk_instance, VulkanWSI& vulkan_wsi) override;

private:
    uint32_t physical_device_index;
};