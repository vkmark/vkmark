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

#include "vulkan_state.h"
#include "device_uuid.h"

#include <array>


void log_info(vk::PhysicalDevice const& physical_device)
{
    auto const props = physical_device.getProperties();

    Log::info("    Vendor ID:      0x%X\n", props.vendorID);
    Log::info("    Device ID:      0x%X\n", props.deviceID);
    Log::info("    Device Name:    %s\n", static_cast<char const*>(props.deviceName));
    Log::info("    Driver Version: %u\n", props.driverVersion);
    Log::info("    Device UUID:    %s\n", static_cast<DeviceUUID>(props.pipelineCacheUUID).representation().data());
}

void log_info(std::vector<vk::PhysicalDevice> const& physical_devices)
{
    for (size_t i = 0; i < physical_devices.size(); ++i)
    {        
        Log::info("=== Physical Device %d. ===\n", i);
        log_info(physical_devices[i]);
    }
}

// pseudo-optional
std::pair<uint32_t, bool> find_queue_family_index(vk::PhysicalDevice pd, vk::QueueFlagBits queue_family_type)
{
    auto const queue_families = pd.getQueueFamilyProperties();
    
    for (uint32_t queue_index = 0; queue_index < queue_families.size(); ++queue_index)
    {
        // each queue family must support at least one queue
        if (queue_families[queue_index].queueFlags & queue_family_type)
            return std::make_pair(queue_index, true);
    }

    return std::make_pair(0, false);
}

void VulkanState::create_instance(VulkanWSI& vulkan_wsi)
{
    auto const app_info = vk::ApplicationInfo{}
        .setPApplicationName("vkmark");

    std::vector<char const*> enabled_extensions{vulkan_wsi.vulkan_extensions()};
    enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

    auto const create_info = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&app_info)
        .setEnabledExtensionCount(enabled_extensions.size())
        .setPpEnabledExtensionNames(enabled_extensions.data());

    vk_instance = ManagedResource<vk::Instance>{
        vk::createInstance(create_info),
        [] (auto& i) { i.destroy(); }};
}

void VulkanState::create_logical_device(VulkanWSI& vulkan_wsi)
{
    // it would be really nice to support c++17
    auto pair = find_queue_family_index(physical_device(), vk::QueueFlagBits::eGraphics);
    if (!pair.second)
        throw std::runtime_error("selected physical device does not provide graphics queue");
    vk_graphics_queue_family_index = pair.first;

    auto const priority = 1.0f;

    auto queue_family_indices =
        vulkan_wsi.physical_device_queue_family_indices(physical_device());

    if (!queue_family_indices.empty())
    {
        Log::debug("VulkanState: Using queue family index %d for WSI operations\n",
                   queue_family_indices.front());
    }

    if (std::find(queue_family_indices.begin(),
                  queue_family_indices.end(),
                  graphics_queue_family_index()) == queue_family_indices.end())
    {
        queue_family_indices.push_back(graphics_queue_family_index());
    }

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    for (auto index : queue_family_indices)
    {
        queue_create_infos.push_back(
            vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(index)
                .setQueueCount(1)
                .setPQueuePriorities(&priority));
    }

    Log::debug("VulkanState: Using queue family index %d for rendering\n",
               graphics_queue_family_index());

    std::array<char const*,1> enabled_extensions{
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    auto const device_features = vk::PhysicalDeviceFeatures{}
        .setSamplerAnisotropy(true);

    auto const device_create_info = vk::DeviceCreateInfo{}
        .setQueueCreateInfoCount(queue_create_infos.size())
        .setPQueueCreateInfos(queue_create_infos.data())
        .setEnabledExtensionCount(enabled_extensions.size())
        .setPpEnabledExtensionNames(enabled_extensions.data())
        .setPEnabledFeatures(&device_features);

    vk_device = ManagedResource<vk::Device>{
        physical_device().createDevice(device_create_info),
        [] (auto& d) { d.destroy(); }};

    vk_graphics_queue = device().getQueue(graphics_queue_family_index(), 0);
}

void VulkanState::create_command_pool()
{
    auto const command_pool_create_info = vk::CommandPoolCreateInfo{}
        .setQueueFamilyIndex(graphics_queue_family_index())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    vk_command_pool = ManagedResource<vk::CommandPool>{
        device().createCommandPool(command_pool_create_info),
        [this] (auto& cp) { this->device().destroyCommandPool(cp); }};
}

vk::PhysicalDevice ChooseFirstSupportedStrategy::operator()(const std::vector<vk::PhysicalDevice>& avaiable_devices)
{
    Log::debug("Trying to use first supported device.\n");

    for (auto const& physical_device : avaiable_devices)
    {
        if (find_queue_family_index(physical_device, vk::QueueFlagBits::eGraphics).second)
        {
            Log::debug("First supported device choosen!\n");
            return physical_device;
        }

        Log::debug("device with uuid %s skipped!\n",
               static_cast<DeviceUUID>(physical_device.getProperties().pipelineCacheUUID).representation().data()
        );
    }
    
    throw std::runtime_error("No suitable Vulkan physical devices found");
}

vk::PhysicalDevice ChooseByUUIDStrategy::operator()(const std::vector<vk::PhysicalDevice>& avaiable_devices)
{
    Log::debug("Trying to use device with specified UUID %s.\n", 
        m_selected_device_uuid.representation().data());

    for (auto const& physical_device: avaiable_devices)
    {
        if (static_cast<DeviceUUID>(physical_device.getProperties().pipelineCacheUUID) == m_selected_device_uuid)
        {
            Log::debug("Device found by UUID\n");
            return physical_device;
        }
    }

    throw std::runtime_error(std::string("Device specified by uuid is not avaiable!"));
}