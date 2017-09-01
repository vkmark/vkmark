/*
 * Copyright © 2010-2012 Linaro Limited
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

#include "log.h"

#include <unistd.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

std::string const Log::continuation_prefix{"\x10"};
std::string Log::appname;
bool Log::do_debug{false};

namespace
{

std::string const terminal_color_normal{"\033[0m"};
std::string const terminal_color_red{"\033[1;31m"};
std::string const terminal_color_cyan{"\033[36m"};
std::string const terminal_color_yellow{"\033[33m"};
std::string const terminal_color_magenta{"\033[35m"};
std::string const empty;

void print_prefixed_message(
    std::ostream& stream,
    std::string const& color,
    std::string const& prefix,
    std::string const& fmt,
    va_list ap)
{
    va_list aq;

    /* Estimate message size */
    va_copy(aq, ap);
    int msg_size = std::vsnprintf(NULL, 0, fmt.c_str(), aq);
    va_end(aq);

    /* Create the buffer to hold the message */
    std::vector<char> buf(msg_size + 1);

    /* Store the message in the buffer */
    va_copy(aq, ap);
    std::vsnprintf(buf.data(), msg_size + 1, fmt.c_str(), aq);
    va_end(aq);

    /*
     * Print the message lines prefixed with the supplied prefix.
     * If the target stream is a terminal make the prefix colored.
     */
    std::string line_prefix;
    if (!prefix.empty())
    {
        static std::string const colon{": "};
        std::string start_color;
        std::string end_color;
        if (!color.empty())
        {
            start_color = color;
            end_color = terminal_color_normal;
        }
        line_prefix = start_color + prefix + end_color + colon;
    }

    std::string line;
    std::stringstream ss{buf.data()};

    while(std::getline(ss, line))
    {
        /*
         * If this line is a continuation of a previous log message
         * just print the line plainly.
         */
        if (line[0] == Log::continuation_prefix[0])
        {
            stream << line.c_str() + 1;
        }
        else
        {
            /* Normal line, emit the prefix. */
            stream << line_prefix << line;
        }

        /* Only emit a newline if the original message has it. */
        if (!(ss.rdstate() & std::stringstream::eofbit))
            stream << std::endl;
    }

}

}

void Log::init(std::string const& appname_, bool do_debug_)
{
    appname = appname_;
    do_debug = do_debug_;
}

void Log::info(char const* fmt, ...)
{
    static std::string const infoprefix{"Info"};
    std::string const& prefix{do_debug ? infoprefix : empty};

    va_list ap;
    va_start(ap, fmt);

    static std::string const& infocolor{isatty(fileno(stdout)) ? terminal_color_cyan : empty};
    std::string const& color{do_debug ? infocolor : empty};
    print_prefixed_message(std::cout, color, prefix, fmt, ap);

    va_end(ap);
}

void Log::debug(const char *fmt, ...)
{
    static std::string const dbgprefix("Debug");
    if (!do_debug)
        return;

    va_list ap;
    va_start(ap, fmt);

    static std::string const& dbgcolor{isatty(fileno(stdout)) ? terminal_color_yellow : empty};
    print_prefixed_message(std::cout, dbgcolor, dbgprefix, fmt, ap);

    va_end(ap);
}

void Log::error(const char *fmt, ...)
{
    static std::string const errprefix("Error");

    va_list ap;
    va_start(ap, fmt);

    static std::string const& errcolor{isatty(fileno(stderr)) ? terminal_color_red : empty};
    print_prefixed_message(std::cerr, errcolor, errprefix, fmt, ap);

    va_end(ap);
}

void Log::warning(const char *fmt, ...)
{
    static std::string const warnprefix("Warning");

    va_list ap;
    va_start(ap, fmt);

    static std::string const& warncolor{isatty(fileno(stderr)) ? terminal_color_magenta : empty};
    print_prefixed_message(std::cerr, warncolor, warnprefix, fmt, ap);

    va_end(ap);
}

void Log::flush()
{
    std::cout.flush();
    std::cerr.flush();
}
