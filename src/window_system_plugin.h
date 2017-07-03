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

#include <memory>

struct Options;
class WindowSystem;

using VkMarkWindowSystemLoadOptionsFunc = void(*)(Options&);
using VkMarkWindowSystemCreateFunc = std::unique_ptr<WindowSystem>(*)(Options const&);
using VkMarkWindowSystemProbeFunc = int(*)(Options const&);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

extern "C"
{

void vkmark_window_system_load_options(Options& options);
int vkmark_window_system_probe(Options const&);
std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options);

}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
