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

#include "log.h"
#include "options.h"

namespace
{

std::string const display_index_opt{"display-index"};

std::string get_display_index_option(Options const& options)
{
    auto iter = std::find_if(
        options.window_system_options.begin(),
        options.window_system_options.end(),
        [] (auto opt) { return opt.name == display_index_opt; });
    return iter != options.window_system_options.end() ?
           iter->value : "";
}

}

void vkmark_window_system_load_options(Options& options)
{
    options.add_window_system_help(
        "Display window system options (pass in --winsys-options)\n"
        "  display-index=INDEX         The index of the Vulkan display to use (default: 0)\n"
        );
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

    try
    {
        auto vk_instance = ManagedResource<vk::Instance>{
            vk::createInstance(create_info),
            [] (auto& i) { i.destroy(); }};
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
            auto display_index = std::stoi(get_display_index_option(options));
            if (display_index < 0)
                throw std::runtime_error{""};
            DisplayNativeSystem::get_display_surface_create_info(*physical_device, display_index);
            return VKMARK_WINDOW_SYSTEM_PROBE_OK + VKMARK_DISPLAY_WINDOW_SYSTEM_PRIORITY;
        }
    }
    catch (...)
    {
    }


    return VKMARK_WINDOW_SYSTEM_PROBE_BAD;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    unsigned int display_index = 0;

    for (auto const& opt : options.window_system_options)
    {
        if (opt.name == display_index_opt)
        {
            try
            {
                auto i = std::stoi(opt.value);
                if (i < 0)
                    throw std::runtime_error{""};
                display_index = i;
            }
            catch (...)
            {
                throw std::runtime_error{"Invalid value for winsys option 'display-index'"};
            }
        }
        else
        {
            Log::info("DisplayWindowSystemPlugin: Ignoring unknown window system option '%s'\n",
                      opt.name.c_str());
        }
    }

    return std::make_unique<SwapchainWindowSystem>(
        std::make_unique<DisplayNativeSystem>(display_index),
        options.present_mode,
        options.pixel_format);
}
