project(
    'vkmark',
    'cpp',
    default_options : ['cpp_std=c++14'],
    version : '2017.06'
    )

ws_dir = join_paths([get_option('prefix'), get_option('libdir'), 'vkmark'])

add_global_arguments('-DVKMARK_WINDOW_SYSTEM_DIR="@0@"'.format(ws_dir), language : 'cpp')
add_global_arguments('-DVKMARK_VERSION_STR="@0@"'.format(meson.project_version()), language : 'cpp')

cpp = meson.get_compiler('cpp')

vulkan_dep = cpp.find_library('vulkan')
dl_dep = cpp.find_library('dl')

xcb_dep = dependency('xcb', required : get_option('xcb') == 'true')
xcb_icccm_dep = dependency('xcb-icccm', required : get_option('xcb') == 'true')
wayland_client_dep = dependency('wayland-client', required : get_option('wayland') == 'true')

build_xcb_ws = xcb_dep.found() and xcb_icccm_dep.found() and get_option('xcb') != 'false'
build_wayland_ws = wayland_client_dep.found() and get_option('wayland') != 'false'

subdir('src')

msg = 'Building with support for the following window systems: '

if build_wayland_ws
    msg += 'wayland '
endif

if build_xcb_ws
    msg += 'xcb '
endif

message(msg)
