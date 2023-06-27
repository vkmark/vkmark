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
#include "kms_window_system.h"
#include "atomic_kms_window_system.h"

#include "options.h"
#include "log.h"

#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>

namespace
{

std::string const drm_device_opt{"kms-drm-device"};
std::string const atomic_opt{"kms-atomic"};
std::string const mod_invalid_tiling_opt{"kms-mod-invalid-tiling"};
std::string const tty_opt{"kms-tty"};

std::string get_drm_device_option(Options const& options)
{
    auto const& winsys_options = options.window_system_options;
    std::string drm_device{"/dev/dri/card0"};

    for (auto const& opt : winsys_options)
    {
        if (opt.name == drm_device_opt)
            drm_device = opt.value;
    }

    return drm_device;
}

}

void vkmark_window_system_load_options(Options& options)
{
    options.add_window_system_help(
        "KMS window system options (pass in --winsys-options)\n"
        "  kms-drm-device=DEV          The drm device to use (default: /dev/dri/card0)\n"
        "  kms-atomic=auto|yes|no      Whether to use atomic modesetting (default: auto)\n"
        "  kms-mod-invalid-tiling=TIL  Tiling mode (optimal|linear) to use for invalid modifier\n"
        "                              (default: optimal)\n"
        "  kms-tty=TTY                 The TTY to use (default: /dev/tty)\n"
        );
}

int vkmark_window_system_probe(Options const& options)
{
    bool supported = false;

    int const drm_fd = open(get_drm_device_option(options).c_str(), O_RDWR);
    if (drm_fd >= 0)
    {
        if (drmSetMaster(drm_fd) >= 0)
        {
            drmDropMaster(drm_fd);
            supported = true;
        }
        close(drm_fd);
    }

    return supported ? 255 : 0;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    auto const& winsys_options = options.window_system_options;
    std::string drm_device{"/dev/dri/card0"};
    std::string atomic{"auto"};
    vk::ImageTiling mod_invalid_tiling{vk::ImageTiling::eOptimal};
    std::string tty{"/dev/tty"};

    for (auto const& opt : winsys_options)
    {
        if (opt.name == drm_device_opt)
        {
            drm_device = opt.value;
        }
        else if (opt.name == atomic_opt)
        {
            if (opt.value != "auto" && opt.value != "yes" && opt.value != "no")
            {
                Log::info("KMSWindowSystemPlugin: Ignoring unknown value '%s'"
                          " for window system option '%s'\n",
                          opt.value.c_str(), opt.name.c_str());
            }
            else
            {
                atomic = opt.value;
            }
        }
        else if (opt.name == mod_invalid_tiling_opt)
        {
            if (opt.value == "optimal")
            {
                mod_invalid_tiling = vk::ImageTiling::eOptimal;
            }
            else if (opt.value == "linear")
            {
                mod_invalid_tiling = vk::ImageTiling::eLinear;
            }
            else
            {
                Log::info("KMSWindowSystemPlugin: Ignoring unknown value '%s'"
                          " for window system option '%s'\n",
                          opt.value.c_str(), opt.name.c_str());
            }
        }
        else if (opt.name == tty_opt)
        {
            tty = opt.value;
        }
        else
        {
            Log::info("KMSWindowSystemPlugin: Ignoring unknown window system option '%s'\n",
                      opt.name.c_str());
        }
    }

    if (atomic == "yes" ||
        (atomic == "auto" && AtomicKMSWindowSystem::is_supported_on(drm_device)))
    {
        Log::debug("KMSWindowSystemPlugin: Using atomic modesetting\n");
        return std::make_unique<AtomicKMSWindowSystem>(drm_device, tty, mod_invalid_tiling);
    }
    else
    {
        Log::debug("KMSWindowSystemPlugin: Using legacy modesetting\n");
        return std::make_unique<KMSWindowSystem>(drm_device, tty, mod_invalid_tiling);
    }
}
