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

#include "window_system_plugin.h"
#include "swapchain_window_system.h"
#include "display_native_system.h"
#include "window_system_priority.h"

#include "options.h"

void vkmark_window_system_load_options(Options& options)
{
}

int vkmark_window_system_probe(Options const& options)
{
    auto const app_info = vk::ApplicationInfo{}
        .setPApplicationName("vkmark")
#ifdef VK_MAKE_API_VERSION
        .setApiVersion(VK_MAKE_API_VERSION(0, 1, 0, 0));
#else
        .setApiVersion(VK_MAKE_VERSION(1, 0, 0));
#endif

    auto const exts = std::array<char const*, 2>{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME,
        };

    auto const create_info = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&app_info)
        .setEnabledExtensionCount(exts.size())
        .setPpEnabledExtensionNames(exts.data());
    auto vk_instance = ManagedResource<vk::Instance>{
        vk::createInstance(create_info),
        [] (auto& i) { i.destroy(); }};
    if (!vk_instance.raw)
        return VKMARK_WINDOW_SYSTEM_PROBE_BAD;

    auto physical_devices = vk_instance.raw.enumeratePhysicalDevices();
    auto physical_device = options.use_device_with_uuid ?
        std::find_if(
            physical_devices.begin(), physical_devices.end(),
            [&options] (auto pd) {
                return static_cast<DeviceUUID>(pd.getProperties().pipelineCacheUUID) ==
                    options.use_device_with_uuid; }) :
        physical_devices.begin();

    if (physical_device != physical_devices.end())
    {
        try
        {
            DisplayNativeSystem::get_display_surface_create_info(*physical_device);
            return VKMARK_WINDOW_SYSTEM_PROBE_OK + VKMARK_DISPLAY_WINDOW_SYSTEM_PRIORITY;
        }
        catch (...)
        {
        }
    }

    return VKMARK_WINDOW_SYSTEM_PROBE_BAD;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    return std::make_unique<SwapchainWindowSystem>(
        std::make_unique<DisplayNativeSystem>(),
        options.present_mode,
        options.pixel_format);
}
