#pragma once

#include <string>
#include <vector>
#include "tool_types.h"
#include "common_utils.h"

namespace BuiltinTools {
namespace PatternMatcher {

// 简单通配符匹配（支持*通配符）
bool matchPattern(const std::string& text, const std::string& pattern);

// 使用正则表达式在文件中搜索匹配内容
std::vector<json> searchInFileRegex(
    const std::string& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches);

} // namespace PatternMatcher
} // namespace BuiltinTools
