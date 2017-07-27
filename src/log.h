/*
 * Copyright © 2011-2012 Linaro Ltd.
 * Copyright © 2017 Collabora Ltd.
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vkmark.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

#pragma once

#include <string>

class Log
{
public:
    static void init(std::string const& appname, bool do_debug = false);

    static void info(const char *fmt, ...);
    static void debug(const char *fmt, ...);
    static void error(const char *fmt, ...);
    static void warning(const char *fmt, ...);
    static void flush();
    static std::string const continuation_prefix;

private:
    static std::string appname;
    static bool do_debug;
};
