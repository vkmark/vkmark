core_sources = files(
    'benchmark.cpp',
    'benchmark_collection.cpp',
    'default_benchmarks.cpp',
    'log.cpp',
    'main.cpp',
    'options.cpp',
    'scene.cpp',
    'scene_collection.cpp',
    'util.cpp',
    'vkutil.cpp',
    'vulkan_state.cpp',
    'window_system_loader.cpp'
    )

scene_sources = files(
    'clear_scene.cpp',
    'cube_scene.cpp',
    'default_options_scene.cpp',
    )

vkmark = executable(
    'vkmark',
    core_sources + scene_sources,
    dependencies : [vulkan_dep, glm_dep, dl_dep],
    link_args: ['-Wl,--dynamic-list=' + join_paths([meson.current_source_dir(), 'dynamic.list'])],
    install : true
    )

if build_xcb_ws
    xcb_ws = shared_module(
        'xcb',
        'xcb_window_system.cpp',
        dependencies : [vulkan_dep, xcb_dep, xcb_icccm_dep],
        name_prefix : '',
        install : true,
        install_dir : ws_dir
        )
endif

if build_wayland_ws
    wayland_ws = shared_module(
        'wayland',
        'wayland_window_system.cpp',
        dependencies : [vulkan_dep, wayland_client_dep],
        name_prefix : '',
        install : true,
        install_dir : ws_dir
        )
endif
