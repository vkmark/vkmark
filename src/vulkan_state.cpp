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

#include "log.h"

#include <vector>

VulkanState::VulkanState(std::vector<char const*> required_extensions)
{
    create_instance(required_extensions);
    choose_physical_device();
    create_device();
    create_command_pool();
}

void VulkanState::log_info()
{
    auto const props = physical_device().getProperties();

    Log::info("    Vendor ID:      0x%X\n", props.vendorID);
    Log::info("    Device ID:      0x%X\n", props.deviceID);
    Log::info("    Device Name:    %s\n", props.deviceName);
    Log::info("    Driver Version: %u\n", props.driverVersion);
}

void VulkanState::create_instance(
    std::vector<char const*> const& required_extensions)
{
    auto const app_info = vk::ApplicationInfo{}
        .setPApplicationName("vkmark");

    std::vector<char const*> enabled_extensions{required_extensions};
    enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

    auto const create_info = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&app_info)
        .setEnabledExtensionCount(enabled_extensions.size())
        .setPpEnabledExtensionNames(enabled_extensions.data());

    vk_instance = ManagedResource<vk::Instance>{
        vk::createInstance(create_info),
        [] (auto& i) { i.destroy(); }};
}

void VulkanState::choose_physical_device()
{
    auto const physical_devices = instance().enumeratePhysicalDevices();

    for (auto const& pd : physical_devices)
    {
        auto const queue_families = pd.getQueueFamilyProperties();
        int queue_index = 0;
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
    }

    if (!vk_physical_device)
        throw std::runtime_error("No suitable Vulkan physical devices found");
}

void VulkanState::create_device()
{
    auto const priority = 1.0f;
    auto const queue_create_info = vk::DeviceQueueCreateInfo{}
        .setQueueFamilyIndex(graphics_queue_family_index())
        .setQueueCount(1)
        .setPQueuePriorities(&priority);

    std::array<char const*,1> enabled_extensions{
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    auto const device_create_info = vk::DeviceCreateInfo{}
        .setQueueCreateInfoCount(1)
        .setPQueueCreateInfos(&queue_create_info)
        .setEnabledExtensionCount(enabled_extensions.size())
        .setPpEnabledExtensionNames(enabled_extensions.data());

    vk_device = ManagedResource<vk::Device>{
        physical_device().createDevice(device_create_info),
        [] (auto& d) { d.destroy(); }};

    vk_graphics_queue = device().getQueue(graphics_queue_family_index(), 0);
}

void VulkanState::create_command_pool()
{
    auto const command_pool_create_info = vk::CommandPoolCreateInfo{}
        .setQueueFamilyIndex(graphics_queue_family_index());

    vk_command_pool = ManagedResource<vk::CommandPool>{
        device().createCommandPool(command_pool_create_info),
        [this] (auto& cp) { this->device().destroyCommandPool(cp); }};
}
