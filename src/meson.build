core_sources = files(
    'benchmark.cpp',
    'benchmark_collection.cpp',
    'default_benchmarks.cpp',
    'log.cpp',
    'mesh.cpp',
    'model.cpp',
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

vkmark_core = static_library(
    'vkmark-core',
    core_sources,
    dependencies : [vulkan_dep, dl_dep, assimp_dep]
    )

vkmark = executable(
    'vkmark',
    files('main.cpp') + scene_sources,
    link_with: vkmark_core,
    dependencies : [vulkan_dep, glm_dep, dl_dep],
    link_args: ['-Wl,--dynamic-list=' + join_paths([meson.current_source_dir(), 'dynamic.list'])],
    install : true
    )

if build_xcb_ws
    xcb_ws = shared_module(
        'xcb',
        'ws/xcb_window_system_plugin.cpp',
        'ws/xcb_native_system.cpp',
        'ws/swapchain_window_system.cpp',
        dependencies : [vulkan_dep, xcb_dep, xcb_icccm_dep],
        name_prefix : '',
        install : true,
        install_dir : ws_dir
        )
endif

if build_wayland_ws
    wayland_ws = shared_module(
        'wayland',
        'ws/wayland_window_system_plugin.cpp',
        'ws/wayland_native_system.cpp',
        'ws/swapchain_window_system.cpp',
        dependencies : [vulkan_dep, wayland_client_dep],
        name_prefix : '',
        install : true,
        install_dir : ws_dir
        )
endif

if build_kms_ws
    kms_ws = shared_module(
        'kms',
        'ws/kms_window_system_plugin.cpp',
        'ws/kms_window_system.cpp',
        dependencies : [vulkan_dep, libdrm_dep, gbm_dep],
        name_prefix : '',
        install : true,
        install_dir : ws_dir
        )
endif
