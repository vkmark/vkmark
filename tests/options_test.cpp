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

#include "src/options.h"
#include "src/util.h"

#include "catch.hpp"
#include <memory>

using namespace Catch::Matchers;

SCENARIO("options prints window system help", "")
{
    Options options;

    GIVEN("Options with window system help")
    {
        std::string const ws1_help{"WINDOW SYSTEM HELP 1\n"};
        std::string const ws2_help{"WINDOW SYSTEM HELP 2\n"};

        options.add_window_system_help(ws1_help);
        options.add_window_system_help(ws2_help);

        WHEN("getting the help string")
        {
            auto const help = options.help_string();

            THEN("the window system help is contained")
            {
                REQUIRE_THAT(help, Contains(ws1_help));
                REQUIRE_THAT(help, Contains(ws2_help));
            }
        }
    }
}

namespace
{

std::unique_ptr<char*[]> argv_from_vector(std::vector<std::string>& vec)
{
    auto argv = std::make_unique<char*[]>(vec.size() + 1);

    for (size_t i = 0; i < vec.size(); ++i)
        argv[i] = &vec[i][0];

    argv[vec.size()] = nullptr;

    return argv;
}

}

SCENARIO("options parses command line arguments", "")
{
    Options options;

    GIVEN("A command line with invalid options fails")
    {
        std::vector<std::string> args{"vkmark", "--invalid"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            auto const result = options.parse_args(args.size(), argv.get());

            THEN("the parsing fails")
            {
                REQUIRE_FALSE(result);
            }
        }
    }
    GIVEN("A command line with -b and --benchmark")
    {
        std::string const bench1{"scene1:opt1=val1"};
        std::string const bench2{"scene2"};
        std::vector<std::string> args{"vkmark", "-b", bench1, "--benchmark", bench2};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the benchmarks are parsed")
            {
                REQUIRE_THAT(options.benchmarks,
                             Equals(std::vector<std::string>{bench1, bench2}));
            }
        }
    }

    GIVEN("A command line with -s")
    {
        int const width = 123;
        int const height = 456;
        std::vector<std::string> args{
            "vkmark", "-s", std::to_string(width) + "x" + std::to_string(height)};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the size is parsed")
            {
                REQUIRE(options.size.first == width);
                REQUIRE(options.size.second == height);
            }
        }
    }

    GIVEN("A command line with --size")
    {
        int const width = 123;
        int const height = 456;
        std::vector<std::string> args{
            "vkmark", "--size", std::to_string(width) + "x" + std::to_string(height)};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the size is parsed")
            {
                REQUIRE(options.size.first == width);
                REQUIRE(options.size.second == height);
            }
        }
    }

    GIVEN("A command line with --fullscreen")
    {
        std::vector<std::string> args{"vkmark", "--fullscreen"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the fullscreen option is parsed")
            {
                REQUIRE(options.size.first == -1);
                REQUIRE(options.size.second == -1);
            }
        }
    }

    GIVEN("A command line with --present-mode")
    {
        std::vector<std::string> args{"vkmark", "--present-mode", "fifo"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the present mode is parsed")
            {
                REQUIRE(options.present_mode == vk::PresentModeKHR::eFifo);
            }
        }
    }

    GIVEN("A command line with --list-scenes")
    {
        std::vector<std::string> args{"vkmark", "--list-scenes"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.list_scenes);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the list-scenes option is parsed")
            {
                REQUIRE(options.list_scenes);
            }
        }
    }

    GIVEN("A command line with --show-all-options")
    {
        std::vector<std::string> args{"vkmark", "--show-all-options"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.show_all_options);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the show-all-options option is parsed")
            {
                REQUIRE(options.show_all_options);
            }
        }
    }

    GIVEN("A command line with --winsys-dir")
    {
        std::string const winsys_dir{"bla/winsys"};
        std::vector<std::string> args{"vkmark", "--winsys-dir", winsys_dir};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the window system dir is parsed")
            {
                REQUIRE(options.window_system_dir == winsys_dir);
            }
        }
    }

    GIVEN("A command line with --data-dir")
    {
        std::string const data_dir{"bla/data"};
        std::vector<std::string> args{"vkmark", "--data-dir", data_dir};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the data dir is parsed")
            {
                REQUIRE(options.data_dir == data_dir);
            }
        }
    }

    GIVEN("A command line with --winsys")
    {
        std::string const winsys{"mywinsys"};
        std::vector<std::string> args{"vkmark", "--winsys", winsys};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.window_system.empty());
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the winsys is parsed")
            {
                REQUIRE(options.window_system == winsys);
            }
        }
    }

    GIVEN("A command line with --winsys-options")
    {
        std::string const winsys_options{"opt1=v1:opt2=v2"};
        std::vector<std::string> args{"vkmark", "--winsys-options", winsys_options};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the data dir is parsed")
            {
                REQUIRE(options.window_system_options.size() == 2);
                REQUIRE(options.window_system_options[0].name == "opt1");
                REQUIRE(options.window_system_options[0].value == "v1");
                REQUIRE(options.window_system_options[1].name == "opt2");
                REQUIRE(options.window_system_options[1].value == "v2");
            }
        }
    }

    GIVEN("A command line with -d")
    {
        std::vector<std::string> args{"vkmark", "-d"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.show_debug);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the debug option is parsed")
            {
                REQUIRE(options.show_debug);
            }
        }
    }

    GIVEN("A command line with --debug")
    {
        std::vector<std::string> args{"vkmark", "--debug"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.show_debug);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the debug option is parsed")
            {
                REQUIRE(options.show_debug);
            }
        }
    }

    GIVEN("A command line with -h")
    {
        std::vector<std::string> args{"vkmark", "-h"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.show_help);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the help option is parsed")
            {
                REQUIRE(options.show_help);
            }
        }
    }

    GIVEN("A command line with --help")
    {
        std::vector<std::string> args{"vkmark", "--help"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.show_help);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the help option is parsed")
            {
                REQUIRE(options.show_help);
            }
        }
    }

    GIVEN("A command line with --run-forever")
    {
        std::vector<std::string> args{"vkmark", "--run-forever"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.run_forever);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("the run-forever option is parsed")
            {
                REQUIRE(options.run_forever);
            }
        }
    }

    GIVEN("A complex command line")
    {
        std::vector<std::string> args{
            "vkmark",
            "--benchmark=scene1:opt=val",
            "-s", "111x222",
            "--data-dir=data",
            "--winsys-dir=build/src",
            "-p", "fiforelaxed"};
        auto argv = argv_from_vector(args);

        WHEN("parsing the args")
        {
            REQUIRE_FALSE(options.show_help);
            REQUIRE(options.parse_args(args.size(), argv.get()));

            THEN("all options are parsed")
            {
                REQUIRE(options.benchmarks.size() == 1);
                REQUIRE(options.benchmarks[0] == "scene1:opt=val");
                REQUIRE(options.size.first == 111);
                REQUIRE(options.size.second == 222);
                REQUIRE(options.data_dir == "data");
                REQUIRE(options.window_system_dir == "build/src");
                REQUIRE(options.present_mode == vk::PresentModeKHR::eFifoRelaxed);
            }
        }
    }
}
