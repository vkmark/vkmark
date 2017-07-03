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

#include "window_system_loader.h"
#include "window_system.h"
#include "window_system_plugin.h"
#include "options.h"
#include "log.h"

#include <dlfcn.h>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>
#include <stdexcept>

namespace
{

void close_lib(void* handle)
{
    if (handle) dlclose(handle);
}

bool has_shared_object_extension(std::string const& name)
{
    static std::string const so{".so"};

    return name.size() > so.size() &&
           std::equal(so.rbegin(), so.rend(), name.rbegin());
}

std::vector<std::string> files_in_dir(std::string dir)
{
    std::vector<std::string> files;

    auto const dirp = opendir(dir.c_str());

    if (dirp != nullptr)
    {
        dirent const* ent;

        while ((ent = readdir(dirp)) != nullptr)
        {
            if (has_shared_object_extension(ent->d_name))
                files.push_back(dir + "/" + ent->d_name);
        }

        closedir(dirp);
    }

    return files;
}

}

WindowSystemLoader::WindowSystemLoader(Options& options)
    : options{options},
      lib_handle{nullptr, close_lib}
{
    Log::debug("WindowSystemLoader: Looking in %s for window system modules\n",
               options.window_system_dir.c_str());
}

WindowSystem& WindowSystemLoader::load_window_system()
{
    if (window_system)
        return *window_system;

    auto const lib = probe_for_best_window_system();

    lib_handle = LibHandle{dlopen(lib.c_str(), RTLD_LAZY), close_lib};

    auto const ws_create = reinterpret_cast<VkMarkWindowSystemCreateFunc>(
        dlsym(lib_handle.get(), "vkmark_window_system_create"));

    if (!ws_create)
        throw std::runtime_error{"Selected window system module doesn't provide a create function"};
    
    window_system = ws_create(options);

    if (!window_system)
        throw std::runtime_error{"Selected window system module failed to create window system"};

    return *window_system;
}

std::string WindowSystemLoader::probe_for_best_window_system()
{
    std::string best_candidate;
    int best_priority = 0;;

    for_each_window_system(
        [&] (std::string const& name, void* handle)
        {
            Log::debug("WindowSystemLoader: Probing %s... ", name.c_str());

            if (handle)
            {
                auto ws_probe = reinterpret_cast<VkMarkWindowSystemProbeFunc>(
                    dlsym(handle, "vkmark_window_system_probe"));
                if (ws_probe)
                {
                    auto priority = ws_probe(options);
                    if (priority > best_priority)
                    {
                        best_candidate = name;
                        best_priority = priority;
                    }

                    auto const fmt = Log::continuation_prefix + "succeeded with priority %d\n";
                    Log::debug(fmt.c_str(), priority);
                }
                else
                {
                    auto const fmt = Log::continuation_prefix + "failed to find probe function: %s\n";
                    Log::debug(fmt.c_str(), dlerror());
                }
            }
            else
            {
                auto const fmt = Log::continuation_prefix + "failed to load file: %s\n";
                Log::debug(fmt.c_str(), dlerror());
            }
        });


    if (!best_candidate.empty())
        return best_candidate;

    throw std::runtime_error{"Failed to find usable window system, try using --winsys-dir"};
}

void WindowSystemLoader::load_window_system_options()
{
    for_each_window_system(
        [&] (std::string const& name, void* handle)
        {
            Log::debug("WindowSystemLoader: Loading options from %s... ", name.c_str());

            if (handle)
            {
                auto add_options = reinterpret_cast<VkMarkWindowSystemLoadOptionsFunc>(
                    dlsym(handle, "vkmark_window_system_load_options"));
                if (add_options)
                {
                    add_options(options);
                    auto const fmt = Log::continuation_prefix + "ok\n";
                    Log::debug(fmt.c_str());
                }
                else
                {
                    auto const fmt = Log::continuation_prefix + "failed to find load options function: %s\n";
                    Log::debug(fmt.c_str(), dlerror());
                }
            }
            else
            {
                auto const fmt = Log::continuation_prefix + "failed to load file: %s\n";
                Log::debug(fmt.c_str(), dlerror());
            }
        });
}

void WindowSystemLoader::for_each_window_system(ForeachCallback const& callback)
{
    auto const candidates = files_in_dir(options.window_system_dir);

    for (auto const& c : candidates)
    {
        auto const handle = LibHandle{dlopen(c.c_str(), RTLD_LAZY), close_lib};
        callback(c, handle.get());
    }
}
