project(
    'vkmark',
    ['cpp', 'c'],
    default_options : ['cpp_std=c++17'],
    version : '2025.01',
    meson_version: '>=0.58'
    )

root_incdir = include_directories('.')
ws_dir = join_paths([get_option('prefix'), get_option('libdir'), 'vkmark'])
data_dir = join_paths([get_option('prefix'), get_option('datadir'), 'vkmark'])

add_global_arguments('-DVKMARK_WINDOW_SYSTEM_DIR="@0@"'.format(ws_dir), language : 'cpp')
add_global_arguments('-DVKMARK_DATA_DIR="@0@"'.format(data_dir), language : 'cpp')
add_global_arguments('-DVKMARK_VERSION_STR="@0@"'.format(meson.project_version()), language : 'cpp')

cpp = meson.get_compiler('cpp')

vulkan_dep = dependency('vulkan')
dl_dep = cpp.find_library('dl')
glm_dep = dependency('glm', required: false)
if not glm_dep.found() and not cpp.has_header('glm/glm.hpp')
    error('Failed to find glm')
endif
assimp_dep = dependency('assimp')

xcb_dep = dependency('xcb', required : get_option('xcb') == 'true')
xcb_icccm_dep = dependency('xcb-icccm', required : get_option('xcb') == 'true')
wayland_client_dep = dependency('wayland-client', required : get_option('wayland') == 'true')
wayland_protocols_dep = dependency('wayland-protocols', version : '>= 1.12',
                                   required : get_option('wayland') == 'true')
wayland_scanner_dep = dependency('wayland-scanner',
                                 required : get_option('wayland') == 'true',
                                 native: true)
libdrm_dep = dependency('libdrm', required : get_option('kms') == 'true')
gbm_dep = dependency('gbm', required : get_option('kms') == 'true')

build_xcb_ws = xcb_dep.found() and xcb_icccm_dep.found() and get_option('xcb') != 'false'
build_wayland_ws = (wayland_client_dep.found() and wayland_protocols_dep.found() and
                    wayland_scanner_dep.found() and get_option('wayland') != 'false')
build_kms_ws = libdrm_dep.found() and gbm_dep.found() and get_option('kms') != 'false'

subdir('src')
subdir('data')
subdir('tests')
subdir('doc')

msg = 'Building with support for the following window systems: headless display '

if build_wayland_ws
    msg += 'wayland '
endif

if build_xcb_ws
    msg += 'xcb '
endif

if build_kms_ws
    msg += 'kms '
endif

message(msg)
