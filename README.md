![](https://github.com/vkmark/vkmark/workflows/build/badge.svg)

vkmark is an extensible Vulkan benchmarking suite with targeted,
configurable scenes.

# Building requirements

To build vkmark you need:

 * the meson build system
 * a C++17 compiler
 * libvulkan and development files
 * libglm development files (header only library)
 * libassimp and development files

for the X11 backend:

 * libxcb and development files
 * libxcb-icccm and development files

for the wayland backend:

 * libwayland-client and development files
 * wayland-protocols >= 1.12

for the KMS backend:

 * libdrm and development files
 * libgbm and development files

On a recent Debian/Ubuntu system you can get all the dependencies with:

 `$ sudo apt install meson libvulkan-dev libglm-dev libassimp-dev libxcb1-dev libxcb-icccm4-dev libwayland-dev wayland-protocols libdrm-dev libgbm-dev`

# Building and installing

vkmark uses the meson build system.

To configure vkmark use:

`$ meson build`

This will create a vkmark build configuration in the **build** directory.

To see/set the full list of available options use:

`$ mesonconf [-Dopt=val] build`

To build use:

`$ ninja -C build`

To install use:

`$ [DESTDIR=...] ninja -C build install`

# Running

After installing you can run vkmark with:

`$ vkmark [options...]`

You can run vkmark from the project directory without installing with:

`$ build/src/vkmark --winsys-dir=build/src --data-dir=data [other-options...]`

# Benchmarks

vkmark offers a suite of scenes that can be used to measure various aspects of
Vulkan performance. The way in which each scene is rendered is configurable
through a set of options. To list the available scenes and their acceptable
options you can use the `-l, --list-scenes` command line option.

In vkmark, a benchmark is defined as a scene plus a set of option values. You
can specify the list and order of the benchmarks to run by using the `-b,
--benchmark` command line option (possibly multiple times). If no benchmarks
are specified, a default set of benchmarks is used. If a benchmark option is
not specified it assumes its default value (listed with `-l, --list-scenes`).

As a special case, a benchmark description string is allowed to not contain a
scene name (i.e. to start with ':'). In this case, any specified option values
are used as the default values for benchmarks following this description
string.

# Benchmark examples

To run the default benchmarks:

`$ vkmark`

To run a benchmark using scene 'vertex' with a 'duration' of '5.0' seconds and
'interleave' set to 'false':

`$ vkmark -b vertex:duration=5.0:interleave=false`

To run a series of benchmarks use the `-b, --benchmark` command line option multiple times:

`$ vkmark -b vertex:duration=5.0 -b clear:color=1.0,0.5,0 -b cube`

To set default option values for benchmarks and run them:

`$ vkmark -b :duration=2.0 -b vertex:interleave=true -b vertex:interleave=false -b :duration=5.0 -b cube`

To set default option values for the default benchmarks and run them:

`$ vkmark -b :duration=2.0`

# Window system selection

vkmark tries to automatically detect the most suitable window system to use. If
this fails, or you want to override the automatic detection mechanism, you can use
the `--winsys` command-line option. For example, to force the XCB window system
use:

`$ vkmark --winsys xcb`
