prog_python = find_program('python3')

vulkan_hpp = join_paths([
    vulkan_dep.get_pkgconfig_variable('includedir', define_variable: ['prefix', get_option('prefix')]),
    'vulkan',
    'vulkan.hpp'
    ])

format_map_gen_h = custom_target(
    'format_map_gen.h',
    output: 'format_map_gen.h',
    input: vulkan_hpp,
    command: [prog_python, files('gen_format_map.py'), '@INPUT@', '@OUTPUT@']
    )

core_sources = files(
    'benchmark.cpp',
    'benchmark_collection.cpp',
    'default_benchmarks.cpp',
    'device_uuid.cpp',
    'log.cpp',
    'main_loop.cpp',
    'mesh.cpp',
    'model.cpp',
    'options.cpp',
    'scene.cpp',
    'scene_collection.cpp',
    'util.cpp',
    'vulkan_state.cpp',
    'window_system_loader.cpp'
    ) + [format_map_gen_h]

vkutil_sources = files(
    'vkutil/buffer_builder.cpp',
    'vkutil/copy_buffer.cpp',
    'vkutil/descriptor_set_builder.cpp',
    'vkutil/find_matching_memory_type.cpp',
    'vkutil/framebuffer_builder.cpp',
    'vkutil/image_builder.cpp',
    'vkutil/image_view_builder.cpp',
    'vkutil/map_memory.cpp',
    'vkutil/one_time_command_buffer.cpp',
    'vkutil/pipeline_builder.cpp',
    'vkutil/render_pass_builder.cpp',
    'vkutil/semaphore_builder.cpp',
    'vkutil/texture_builder.cpp',
    'vkutil/transition_image_layout.cpp'
    )

scene_sources = files(
    'scenes/clear_scene.cpp',
    'scenes/cube_scene.cpp',
    'scenes/default_options_scene.cpp',
    'scenes/desktop_scene.cpp',
    'scenes/effect2d_scene.cpp',
    'scenes/shading_scene.cpp',
    'scenes/texture_scene.cpp',
    'scenes/vertex_scene.cpp',
    )

vkmark_core = static_library(
    'vkmark-core',
    core_sources,
    dependencies : [vulkan_dep, dl_dep, assimp_dep]
    )

vkmark = executable(
    'vkmark',
    files('main.cpp') + vkutil_sources + scene_sources,
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
    wayland_scanner = find_program(wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner'))
    wayland_protocols_dir = wayland_protocols_dep.get_pkgconfig_variable('pkgdatadir')

    xdg_shell_xml_path = wayland_protocols_dir + '/stable/xdg-shell/xdg-shell.xml'
    xdg_shell_client_header = custom_target(
        'xdg-shell client-header',
        command: [ wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@' ],
        input: xdg_shell_xml_path,
        output: 'xdg-shell-client-protocol.h',
        )
    xdg_shell_private_code = custom_target(
        'xdg-shell private-code',
        command: [ wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@' ],
        input: xdg_shell_xml_path,
        output: 'xdg-shell-protocol.c',
        )

    wayland_ws = shared_module(
        'wayland',
        'ws/wayland_window_system_plugin.cpp',
        'ws/wayland_native_system.cpp',
        'ws/swapchain_window_system.cpp',
        xdg_shell_client_header,
        xdg_shell_private_code,
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
        'ws/atomic_kms_window_system.cpp',
        dependencies : [vulkan_dep, libdrm_dep, gbm_dep],
        name_prefix : '',
        install : true,
        install_dir : ws_dir
        )
endif
