# QCopilot builtin_tools unit tests

cmake_minimum_required(VERSION 3.14)

# 静态库：打包 builtin_tools 的实现，便于测试可复用
add_library(qcopilot_builtins STATIC
    ../qcopilot_utils.cpp
    ../builtin_tools/common/common_utils_internal.cpp
    ../builtin_tools/common/encoding_utils.cpp
    ../builtin_tools/common/filesystem_utils.cpp
    ../builtin_tools/common/io_utils.cpp
    ../builtin_tools/common/json_utils.cpp
    ../builtin_tools/common/string_utils.cpp
    ../builtin_tools/common/time_utils.cpp
    ../builtin_tools/common/tool_response.cpp
    ../builtin_tools/timeTools/get_current_time.cpp
    ../builtin_tools/fileTools/read_text_lines.cpp
    ../builtin_tools/fileTools/write_text_file.cpp
    ../builtin_tools/fileTools/validate_utf8_file.cpp
    ../builtin_tools/mathTools/mathTools_utils.cpp
    ../builtin_tools/mathTools/calculate.cpp
    ../builtin_tools/mathTools/expression_parser.cpp
    ../builtin_tools/systemTools/list_directory.cpp
    ../builtin_tools/systemTools/path_stat.cpp
    ../builtin_tools/systemTools/glob.cpp
    ../builtin_tools/systemTools/grep.cpp
)

target_include_directories(qcopilot_builtins PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../builtin_tools
    ${CMAKE_SOURCE_DIR}/vendor/nlohmann
)

target_compile_features(qcopilot_builtins PUBLIC cxx_std_17)

# 公共包含路径
set(QCOPILOT_TEST_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../builtin_tools
    ${CMAKE_SOURCE_DIR}/vendor/nlohmann
)

# 定义一个简化宏用于创建单元测试
function(qcopilot_add_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE qcopilot_builtins)
    target_include_directories(${name} PRIVATE ${QCOPILOT_TEST_INCLUDES})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# 各工具独立测试
qcopilot_add_test(test_fileTools_write_text_file        test_support.h test_fileTools_write_text_file.cpp)
qcopilot_add_test(test_fileTools_validate_utf8_file     test_support.h test_fileTools_validate_utf8_file.cpp)
qcopilot_add_test(test_fileTools_read_text_lines        test_support.h test_fileTools_read_text_lines.cpp)
qcopilot_add_test(test_systemTools_glob                 test_support.h test_systemTools_glob.cpp)
qcopilot_add_test(test_systemTools_list_directory       test_support.h test_systemTools_list_directory.cpp)
qcopilot_add_test(test_systemTools_grep                 test_support.h test_systemTools_grep.cpp)
qcopilot_add_test(test_systemTools_path_stat            test_support.h test_systemTools_path_stat.cpp)
qcopilot_add_test(test_timeTools_get_current_time       test_support.h test_timeTools_get_current_time.cpp)
qcopilot_add_test(test_mathTools_calculate              test_support.h test_mathTools_calculate.cpp)

# common 工具组测试
qcopilot_add_test(test_common_validate_existing_path              test_support.h test_common_validate_existing_path.cpp)
qcopilot_add_test(test_common_sanitize_string_for_json     test_support.h test_common_sanitize_string_for_json.cpp)
qcopilot_add_test(test_common_get_current_timestamp       test_support.h test_common_get_current_timestamp.cpp)
qcopilot_add_test(test_common_is_valid_utf8_string         test_support.h test_common_is_valid_utf8_string.cpp)
qcopilot_add_test(test_common_file_exists                test_support.h test_common_file_exists.cpp)
qcopilot_add_test(test_common_format_file_size            test_support.h test_common_format_file_size.cpp)
qcopilot_add_test(test_common_is_regular_readable_file  test_support.h test_common_is_regular_readable_file.cpp)
qcopilot_add_test(test_common_read_file_content           test_support.h test_common_read_file_content.cpp)
qcopilot_add_test(test_common_read_text_with_range     test_support.h test_common_read_text_with_range.cpp)
qcopilot_add_test(test_common_write_file_content          test_support.h test_common_write_file_content.cpp)
qcopilot_add_test(test_common_append_file_content         test_support.h test_common_append_file_content.cpp)
qcopilot_add_test(test_common_list_directory             test_support.h test_common_list_directory.cpp)
qcopilot_add_test(test_common_glob_paths                 test_support.h test_common_glob_paths.cpp)
qcopilot_add_test(test_common_search_in_file_regex         test_support.h test_common_search_in_file_regex.cpp)
qcopilot_add_test(test_common_open_ifstream_unicode     test_support.h test_common_open_ifstream_unicode.cpp)
qcopilot_add_test(test_common_utf8_to_path                test_support.h test_common_utf8_to_path.cpp)
qcopilot_add_test(test_common_path_to_utf8_string          test_support.h test_common_path_to_utf8_string.cpp)
qcopilot_add_test(test_common_is_likely_binaryString      test_support.h test_common_is_likely_binaryString.cpp)
qcopilot_add_test(test_common_split_string               test_support.h test_common_split_string.cpp)
qcopilot_add_test(test_common_trim_string                test_support.h test_common_trim_string.cpp)
qcopilot_add_test(test_common_join_strings               test_support.h test_common_join_strings.cpp)
qcopilot_add_test(test_common_prepare_writable_path      test_support.h test_common_prepare_writable_path.cpp)
qcopilot_add_test(test_common_make_error       test_support.h test_common_make_error.cpp)
qcopilot_add_test(test_common_make_success     test_support.h test_common_make_success.cpp)
qcopilot_add_test(test_common_tool_response             test_support.h test_common_tool_response.cpp)
qcopilot_add_test(test_common_safe_parse_json             test_support.h test_common_safe_parse_json.cpp)
qcopilot_add_test(test_common_format_json                test_support.h test_common_format_json.cpp)
qcopilot_add_test(test_common_get_current_time_ms          test_support.h test_common_get_current_time_ms.cpp)
qcopilot_add_test(test_common_format_timestamp           test_support.h test_common_format_timestamp.cpp)
qcopilot_add_test(test_common_is_valid_utf8_file           test_support.h test_common_is_valid_utf8_file.cpp)
qcopilot_add_test(test_common_is_likely_binary            test_support.h test_common_is_likely_binary.cpp)
qcopilot_add_test(test_common_utils_internal_to_lower_unsafe        test_support.h test_common_utils_internal_to_lower_unsafe.cpp)
qcopilot_add_test(test_common_utils_internal_normalize_case         test_support.h test_common_utils_internal_normalize_case.cpp)
qcopilot_add_test(test_common_utils_internal_slashify               test_support.h test_common_utils_internal_slashify.cpp)
qcopilot_add_test(test_common_utils_internal_match_char_class         test_support.h test_common_utils_internal_match_char_class.cpp)
qcopilot_add_test(test_common_utils_internal_glob_segment_match       test_support.h test_common_utils_internal_glob_segment_match.cpp)
qcopilot_add_test(test_common_utils_internal_split_pattern_segments   test_support.h test_common_utils_internal_split_pattern_segments.cpp)
qcopilot_add_test(test_common_utils_internal_append_replacement_char test_support.h test_common_utils_internal_append_replacement_char.cpp)
qcopilot_add_test(test_common_utils_internal_is_cont                test_support.h test_common_utils_internal_is_cont.cpp)
