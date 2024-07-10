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
#include "log.h"

#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace
{

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    Log::debug("%s\n", callback_data->pMessage);
    return VK_FALSE;
}

class DebugUtilsDispatcher
{
public:
    DebugUtilsDispatcher(vk::Instance& instance) :
        vkCreateDebugUtilsMessengerEXT(
            PFN_vkCreateDebugUtilsMessengerEXT(
                vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))),
        vkDestroyDebugUtilsMessengerEXT(
            PFN_vkDestroyDebugUtilsMessengerEXT(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")))
    {
    }

    size_t getVkHeaderVersion() const
    {
        return VK_HEADER_VERSION;
    }

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
};

}

VulkanState::VulkanState(VulkanWSI& vulkan_wsi, ChoosePhysicalDeviceStrategy const& pd_strategy, bool debug)
    : debug_enabled(debug)
{
    create_instance(vulkan_wsi);
    create_physical_device(vulkan_wsi, pd_strategy);
    create_logical_device(vulkan_wsi);
    create_command_pool();
}

std::vector<vk::PhysicalDevice> VulkanState::available_devices(VulkanWSI& vulkan_wsi) const
{
    auto available_devices = instance().enumeratePhysicalDevices();
    for (auto it_device = available_devices.begin(); it_device != available_devices.end();)
    {
        if (!vulkan_wsi.is_physical_device_supported(*it_device))
        {
            Log::debug("Device with uuid %s is not supported by window system integration layer\n",
                static_cast<DeviceUUID>(it_device->getProperties().pipelineCacheUUID).representation().data());
            it_device = available_devices.erase(it_device);
        }
        else
            ++it_device;
    }

    return available_devices;
}

static void log_device_info(vk::PhysicalDevice const& device)
{
    auto const props = device.getProperties();

    Log::info("    Vendor ID:      0x%X\n", props.vendorID);
    Log::info("    Device ID:      0x%X\n", props.deviceID);
    Log::info("    Device Name:    %s\n", static_cast<char const*>(props.deviceName));
    Log::info("    Driver Version: %u\n", props.driverVersion);
    Log::info("    Device UUID:    %s\n", static_cast<DeviceUUID>(props.pipelineCacheUUID).representation().data());
}

void VulkanState::log_info() const
{
    log_device_info(physical_device());
}

void VulkanState::log_all_devices() const
{
    std::vector<vk::PhysicalDevice> const& physical_devices = instance().enumeratePhysicalDevices();

    for (size_t i = 0; i < physical_devices.size(); ++i)
    {
        Log::info("=== Physical Device %d ===\n", i);
        log_device_info(physical_devices[i]);
    }
}

void VulkanState::create_instance(VulkanWSI& vulkan_wsi)
{
    auto const app_info = vk::ApplicationInfo{}
        .setPApplicationName("vkmark");

    std::vector<char const*> enabled_extensions{vulkan_wsi.required_extensions().instance};
    enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

    std::vector<char const*> validation_layers;

    bool have_debug_extensions = false;
    if (debug_enabled)
    {
        std::vector<vk::LayerProperties> instance_layer_props = vk::enumerateInstanceLayerProperties();

        for (auto layer : instance_layer_props)
        {
            if(strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0)
            {
                have_debug_extensions = true;
                validation_layers.push_back("VK_LAYER_KHRONOS_validation");
                enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                break;
            }
        }
    }

    auto const create_info = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&app_info)
        .setEnabledLayerCount(validation_layers.size())
        .setPpEnabledLayerNames(validation_layers.data())
        .setEnabledExtensionCount(enabled_extensions.size())
        .setPpEnabledExtensionNames(enabled_extensions.data());

    vk_instance = ManagedResource<vk::Instance>{
        vk::createInstance(create_info),
        [] (auto& i) { i.destroy(); }};

    if (have_debug_extensions)
    {
        auto const debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT{}
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(debug_callback);
        auto const dud = DebugUtilsDispatcher{vk_instance};

        debug_messenger = ManagedResource<vk::DebugUtilsMessengerEXT>{
            instance().createDebugUtilsMessengerEXT(debug_create_info, nullptr, dud),
            [this, dud] (auto& d) {instance().destroyDebugUtilsMessengerEXT(d, nullptr, dud);}};
    }
    else
    {
        Log::debug("VK_LAYER_KHRONOS_validation is not supported\n");
    }
}

void VulkanState::create_physical_device(VulkanWSI& vulkan_wsi, ChoosePhysicalDeviceStrategy const& pd_strategy)
{
    vk_physical_device = pd_strategy(available_devices(vulkan_wsi));
}

// pseudo-optional
static std::pair<uint32_t, bool> find_queue_family_index(vk::PhysicalDevice pd, vk::QueueFlagBits queue_family_type)
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

    std::vector<char const*> enabled_extensions{vulkan_wsi.required_extensions().device};

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


vk::PhysicalDevice ChooseFirstSupportedStrategy::operator()(const std::vector<vk::PhysicalDevice>& available_devices)
{
    Log::debug("Trying to use first supported device\n");

    for (auto const& physical_device : available_devices)
    {
        if (find_queue_family_index(physical_device, vk::QueueFlagBits::eGraphics).second)
        {
            Log::debug("First supported device chosen\n");
            return physical_device;
        }

        Log::debug("Device with uuid %s skipped\n",
               static_cast<DeviceUUID>(physical_device.getProperties().pipelineCacheUUID).representation().data()
        );
    }

    throw std::runtime_error("No suitable Vulkan physical devices found");
}

vk::PhysicalDevice ChooseByUUIDStrategy::operator()(const std::vector<vk::PhysicalDevice>& available_devices)
{
    Log::debug("Trying to use device with specified UUID %s\n",
        m_selected_device_uuid.representation().data());

    for (auto const& physical_device: available_devices)
    {
        auto&& uuid = static_cast<DeviceUUID>(physical_device.getProperties().pipelineCacheUUID);
        if (uuid == m_selected_device_uuid)
        {
            Log::debug("Device found by UUID\n");
            return physical_device;
        }
    }

    // if device is not supported by wsi it would appear in list_all_devices but is not available here
    throw std::runtime_error(std::string("Device specified by uuid is not available"));
}
