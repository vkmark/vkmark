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
#include "vulkan_wsi.h"

#include "log.h"

#include <vector>
#include <algorithm>

void log_info(vk::PhysicalDevice const& physical_device)
{
    auto const props = physical_device.getProperties();

    Log::info("    Vendor ID:      0x%X\n", props.vendorID);
    Log::info("    Device ID:      0x%X\n", props.deviceID);
    Log::info("    Device Name:    %s\n", static_cast<char const*>(props.deviceName));
    Log::info("    Driver Version: %u\n", props.driverVersion);
}

void log_info(std::vector<vk::PhysicalDevice> const& physical_devices)
{
    for (size_t device_index = 0; device_index < physical_devices.size(); ++device_index)
    {
        Log::info("=== Physical Device %d. ===\n", device_index);
        log_info(physical_devices[device_index]);
    }
}

VulkanState::VulkanState(VulkanWSI& vulkan_wsi, ChoosePhysicalDeviceStrategy& choose_physical_device_strategy)
{
    create_instance(vulkan_wsi);

    choose_physical_device_strategy.choose(instance(), vulkan_wsi);
    vk_physical_device = choose_physical_device_strategy.physical_device();
    vk_graphics_queue_family_index = choose_physical_device_strategy.graphics_queue_family_index();

    create_device(vulkan_wsi);
    create_command_pool();
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

void VulkanState::create_device(VulkanWSI& vulkan_wsi)
{
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

void ChooseFirstSupportedPhysicalDevice::choose(vk::Instance const& vk_instance, VulkanWSI& vulkan_wsi)
{
    Log::debug("Trying to use first supported device.\n");

    auto const physical_devices = vk_instance.enumeratePhysicalDevices();

    for (auto const& pd : physical_devices)
    {
        if (!vulkan_wsi.is_physical_device_supported(pd))
            continue;

        auto const queue_families = pd.getQueueFamilyProperties();
        uint32_t queue_index = 0;
        for (auto const& queue_family : queue_families)
        {
            if (queue_family.queueCount > 0 &&
                (queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
            {
                vk_physical_device = pd;
                vk_graphics_queue_family_index = queue_index;

                break;
            }
            ++queue_index;
        }

        if (vk_physical_device)
            break;
    }
    
    if (!vk_physical_device)
        throw std::runtime_error("No suitable Vulkan physical devices found");

    Log::debug("First supported device choosen!\n");
}

void ChooseIndexPhysicalDevice::choose(vk::Instance const& vk_instance, VulkanWSI& vulkan_wsi)
{
    auto const physical_devices = vk_instance.enumeratePhysicalDevices();

    // TODO: move to print list of physical devices
    // Log::debug("Found %d physical devices\n", physical_devices.size());

    Log::debug("Trying to use device with specified index %d.\n", use_physical_device_index);

    if (use_physical_device_index < physical_devices.size())
    {
        auto const pd = physical_devices[use_physical_device_index];

        if (vulkan_wsi.is_physical_device_supported(pd))
        {
            auto const queue_families = pd.getQueueFamilyProperties();
            uint32_t queue_index = 0;
            for (auto const& queue_family : queue_families)
            {
                if (queue_family.queueCount > 0 &&
                    (queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    vk_physical_device = pd;
                    vk_graphics_queue_family_index = queue_index;

                    break;
                }
                ++queue_index;
            }

            if(!vk_physical_device)
                Log::warning("Selected device does not support eGraphics queue family!\n");
        }
        else
            Log::warning("Selected device is not supported by your Vulkan window system integration layer (VulkanWSI)!\n");
        
    }
    else
        Log::warning("Device with index %d does not exist!\n", use_physical_device_index);

    if(!vk_physical_device)
    {
       throw std::runtime_error("Could not use device with index " + std::to_string(use_physical_device_index) + "!\n");
    }

    Log::debug("Device with index %d succesfully choosen!\n", use_physical_device_index);
}