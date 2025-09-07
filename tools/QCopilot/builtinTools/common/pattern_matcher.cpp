#include "pattern_matcher.h"
#include <regex>
#include <sstream>
#include <algorithm>

namespace BuiltinTools {
namespace PatternMatcher {

bool matchPattern(const std::string& text, const std::string& pattern) {
    // 简化的模式匹配实现，支持*通配符
    if (pattern == "*") return true;

    size_t star_pos = pattern.find('*');
    if (star_pos == std::string::npos) {
        // 没有通配符，直接比较
        return text == pattern;
    }

    if (star_pos == 0) {
        // *在开头
        std::string suffix = pattern.substr(1);
        return text.length() >= suffix.length() &&
               text.substr(text.length() - suffix.length()) == suffix;
    } else if (star_pos == pattern.length() - 1) {
        // *在末尾
        std::string prefix = pattern.substr(0, star_pos);
        return text.length() >= prefix.length() &&
               text.substr(0, prefix.length()) == prefix;
    } else {
        // *在中间
        std::string prefix = pattern.substr(0, star_pos);
        std::string suffix = pattern.substr(star_pos + 1);
        return text.length() >= prefix.length() + suffix.length() &&
               text.substr(0, prefix.length()) == prefix &&
               text.substr(text.length() - suffix.length()) == suffix;
    }
}

std::vector<json> searchInFileRegex(
    const std::string& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches) {

    std::vector<json> matches;
    std::string content;

    if (!BuiltinTools::Utils::readFileContent(filepath, content)) {
        return matches;
    }

    // 准备正则表达式（如果需要）
    std::regex regex_pattern;
    if (use_regex) {
        try {
            auto flags = std::regex::ECMAScript;
            if (!case_sensitive) {
                flags |= std::regex::icase;
            }
            regex_pattern = std::regex(pattern, flags);
        } catch (const std::regex_error& e) {
            // Invalid regex pattern
            return matches;
        }
    }

    std::istringstream iss(content);
    std::string line;
    int line_num = 1;

    while (std::getline(iss, line) && total_matches < max_matches) {
        bool found = false;

        if (use_regex) {
            found = std::regex_search(line, regex_pattern);
        } else {
            // 子串搜索
            std::string search_line = line;
            std::string search_pattern = pattern;

            if (!case_sensitive) {
                std::transform(search_line.begin(), search_line.end(), search_line.begin(), ::tolower);
                std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(), ::tolower);
            }

            found = (search_line.find(search_pattern) != std::string::npos);
        }

        if (found) {
            json match = {
                {"file", filepath},
                {"line_content", line}
            };

            if (line_numbers) {
                match["line_number"] = line_num;
            }

            matches.push_back(match);
            total_matches++;

            if (total_matches >= max_matches) {
                break;
            }
        }
        line_num++;
    }

    return matches;
}

} // namespace PatternMatcher
} // namespace BuiltinTools
