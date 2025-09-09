# QCopilot builtinTools unit tests

cmake_minimum_required(VERSION 3.14)

# 静态库：打包 builtinTools 的实现，便于测试可复用
add_library(qcopilot_builtins STATIC
    ../qcopilot_utils.cpp
    ../builtinTools/common/common_utils.cpp
    ../builtinTools/common/common_utils_internal.cpp
    ../builtinTools/timeTools/get_current_time.cpp
    ../builtinTools/fileTools/read_text_lines.cpp
    ../builtinTools/fileTools/write_text_file.cpp
    ../builtinTools/fileTools/validate_utf8_file.cpp
    ../builtinTools/mathTools/mathTools_utils.cpp
    ../builtinTools/mathTools/calculate.cpp
    ../builtinTools/mathTools/expression_parser.cpp
    ../builtinTools/systemTools/list_directory.cpp
    ../builtinTools/systemTools/path_stat.cpp
    ../builtinTools/systemTools/glob.cpp
    ../builtinTools/systemTools/grep.cpp
)

target_include_directories(qcopilot_builtins PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../builtinTools
    ${CMAKE_SOURCE_DIR}/vendor/nlohmann
)

target_compile_features(qcopilot_builtins PUBLIC cxx_std_17)

# 公共包含路径
set(QCOPILOT_TEST_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../builtinTools
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
qcopilot_add_test(test_common_validatePath              test_support.h test_common_validatePath.cpp)
qcopilot_add_test(test_common_sanitizeStringForJson     test_support.h test_common_sanitizeStringForJson.cpp)
qcopilot_add_test(test_common_getCurrentTimestamp       test_support.h test_common_getCurrentTimestamp.cpp)
qcopilot_add_test(test_common_matchPattern              test_support.h test_common_matchPattern.cpp)
qcopilot_add_test(test_common_isValidUtf8String         test_support.h test_common_isValidUtf8String.cpp)
qcopilot_add_test(test_common_fileExists                test_support.h test_common_fileExists.cpp)
qcopilot_add_test(test_common_formatFileSize            test_support.h test_common_formatFileSize.cpp)
qcopilot_add_test(test_common_is_regular_readable_file  test_support.h test_common_is_regular_readable_file.cpp)
qcopilot_add_test(test_common_readFileContent           test_support.h test_common_readFileContent.cpp)
qcopilot_add_test(test_common_readTextFileWithRange     test_support.h test_common_readTextFileWithRange.cpp)
qcopilot_add_test(test_common_writeFileContent          test_support.h test_common_writeFileContent.cpp)
qcopilot_add_test(test_common_appendFileContent         test_support.h test_common_appendFileContent.cpp)
qcopilot_add_test(test_common_listDirectory             test_support.h test_common_listDirectory.cpp)
qcopilot_add_test(test_common_globFiles                 test_support.h test_common_globFiles.cpp)
qcopilot_add_test(test_common_searchInFileRegex         test_support.h test_common_searchInFileRegex.cpp)
qcopilot_add_test(test_common_open_ifstream_unicode     test_support.h test_common_open_ifstream_unicode.cpp)
qcopilot_add_test(test_common_utf8ToPath                test_support.h test_common_utf8ToPath.cpp)
qcopilot_add_test(test_common_pathToUtf8String          test_support.h test_common_pathToUtf8String.cpp)
qcopilot_add_test(test_common_isLikelyBinaryString      test_support.h test_common_isLikelyBinaryString.cpp)
qcopilot_add_test(test_common_splitString               test_support.h test_common_splitString.cpp)
qcopilot_add_test(test_common_trimString                test_support.h test_common_trimString.cpp)
qcopilot_add_test(test_common_joinStrings               test_support.h test_common_joinStrings.cpp)
qcopilot_add_test(test_common_createErrorResponse       test_support.h test_common_createErrorResponse.cpp)
qcopilot_add_test(test_common_createSuccessResponse     test_support.h test_common_createSuccessResponse.cpp)
qcopilot_add_test(test_common_safeParseJson             test_support.h test_common_safeParseJson.cpp)
qcopilot_add_test(test_common_formatJson                test_support.h test_common_formatJson.cpp)
qcopilot_add_test(test_common_getCurrentTimeMs          test_support.h test_common_getCurrentTimeMs.cpp)
qcopilot_add_test(test_common_formatTimeStamp           test_support.h test_common_formatTimeStamp.cpp)
qcopilot_add_test(test_common_isValidUtf8File           test_support.h test_common_isValidUtf8File.cpp)
qcopilot_add_test(test_common_isLikelyBinary            test_support.h test_common_isLikelyBinary.cpp)
qcopilot_add_test(test_common_utils_internal_to_lower_unsafe        test_support.h test_common_utils_internal_to_lower_unsafe.cpp)
qcopilot_add_test(test_common_utils_internal_normalize_case         test_support.h test_common_utils_internal_normalize_case.cpp)
qcopilot_add_test(test_common_utils_internal_slashify               test_support.h test_common_utils_internal_slashify.cpp)
qcopilot_add_test(test_common_utils_internal_matchCharClass         test_support.h test_common_utils_internal_matchCharClass.cpp)
qcopilot_add_test(test_common_utils_internal_globSegmentMatch       test_support.h test_common_utils_internal_globSegmentMatch.cpp)
qcopilot_add_test(test_common_utils_internal_splitPatternSegments   test_support.h test_common_utils_internal_splitPatternSegments.cpp)
qcopilot_add_test(test_common_utils_internal_append_replacement_char test_support.h test_common_utils_internal_append_replacement_char.cpp)
qcopilot_add_test(test_common_utils_internal_is_cont                test_support.h test_common_utils_internal_is_cont.cpp)
