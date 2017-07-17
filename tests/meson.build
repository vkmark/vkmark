test_data_dir = join_paths([meson.current_source_dir(), 'data'])
test_window_system_dir = meson.current_build_dir()

test_sources = files(
    'catch_main.cpp',
    'test_scene.cpp',

    'benchmark_collection_test.cpp',
    'managed_resource_test.cpp',
    'mesh_test.cpp',
    'model_test.cpp',
    'options_test.cpp',
    'scene_collection_test.cpp',
    'util_data_file_test.cpp',
    'util_split_test.cpp',
    'window_system_loader_test.cpp',
    )

testws1 = shared_module(
    'testws1',
    'test_window_system_plugin.cpp',
    name_prefix: '',
    include_directories: root_incdir,
    cpp_args : '-DVKMARK_TEST_WINDOW_SYSTEM_ID="1"'
    )
testws2 = shared_module(
    'testws2',
    'test_window_system_plugin.cpp',
    name_prefix: '',
    include_directories: root_incdir,
    cpp_args : '-DVKMARK_TEST_WINDOW_SYSTEM_ID="2"'
    )
testws3 = shared_module(
    'testws3',
    'test_window_system_plugin.cpp',
    name_prefix: '',
    include_directories: root_incdir,
    cpp_args : '-DVKMARK_TEST_WINDOW_SYSTEM_ID="3"'
    )

vkmark_tests = executable(
    'vkmark-tests',
    test_sources,
    link_with: vkmark_core,
    include_directories: root_incdir,
    cpp_args : ['-DVKMARK_TEST_DATA_DIR="@0@"'.format(test_data_dir),
                '-DVKMARK_TEST_WINDOW_SYSTEM_DIR="@0@"'.format(test_window_system_dir)]
    )

test('vkmark tests', vkmark_tests)
