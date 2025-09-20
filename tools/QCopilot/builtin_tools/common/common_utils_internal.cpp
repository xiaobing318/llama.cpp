#include "common_utils_internal.h"

#include "io_utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

namespace builtin_tools::internal {

namespace {

using builtin_tools::common::open_ifstream_unicode;

} // namespace

char to_lower_unsafe(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

std::string normalize_case(std::string s, bool case_sensitive) {
    if (!case_sensitive) {
        std::transform(s.begin(), s.end(), s.begin(), to_lower_unsafe);
    }
    return s;
}

std::string slashify(std::string s) {
#ifdef _WIN32
    std::replace(s.begin(), s.end(), '\\', '/');
#endif
    return s;
}

bool match_char_class(char c, const std::string& cls, bool case_sensitive) {
    char value = c;
    if (!case_sensitive) {
        value = to_lower_unsafe(value);
    }

    bool negate = false;
    std::size_t index = 0;
    if (index < cls.size() && cls[index] == '!') {
        negate = true;
        ++index;
    }

    bool ok = false;
    while (index < cls.size()) {
        char first = cls[index++];
        if (!case_sensitive) {
            first = to_lower_unsafe(first);
        }
        if (index + 1 < cls.size() && cls[index] == '-') {
            ++index;
            char last = cls[index++];
            if (!case_sensitive) {
                last = to_lower_unsafe(last);
            }
            if (first <= value && value <= last) {
                ok = true;
            }
        } else {
            if (first == value) {
                ok = true;
            }
        }
    }
    return negate ? !ok : ok;
}

bool glob_segment_match(const std::string& text, const std::string& pattern, bool case_sensitive) {
    const std::string target = case_sensitive ? text : normalize_case(text, false);
    const std::string pat = case_sensitive ? pattern : normalize_case(pattern, false);

    std::size_t ti = 0;
    std::size_t pi = 0;
    std::size_t star_pi = std::string::npos;
    std::size_t star_ti = 0;

    while (ti < target.size()) {
        if (pi < pat.size()) {
            if (pat[pi] == '?') {
                ++pi;
                ++ti;
                continue;
            }
            if (pat[pi] == '\\') {
                ++pi;
                if (pi < pat.size() && pat[pi] == target[ti]) {
                    ++pi;
                    ++ti;
                    continue;
                }
            } else if (pat[pi] == '[') {
                std::size_t end = pat.find(']', pi + 1);
                if (end == std::string::npos) {
                    return false;
                }
                std::string cls = pat.substr(pi + 1, end - (pi + 1));
                char tc = text[ti];
                if (!case_sensitive) {
                    tc = to_lower_unsafe(tc);
                }
                if (!match_char_class(tc, cls, case_sensitive)) {
                    if (star_pi != std::string::npos) {
                        ti = ++star_ti;
                        pi = star_pi + 1;
                        continue;
                    }
                    return false;
                }
                pi = end + 1;
                ++ti;
                continue;
            } else if (pat[pi] == '*') {
                star_pi = pi;
                star_ti = ti;
                ++pi;
                continue;
            } else if (pat[pi] == target[ti]) {
                ++pi;
                ++ti;
                continue;
            }
        }
        if (star_pi != std::string::npos) {
            ti = ++star_ti;
            pi = star_pi + 1;
            continue;
        }
        return false;
    }
    while (pi < pat.size() && pat[pi] == '*') {
        ++pi;
    }
    return pi == pat.size();
}

std::vector<std::string> split_pattern_segments(std::string pattern) {
    pattern = slashify(std::move(pattern));
    std::vector<std::string> segments;
    std::string current;
    for (char ch : pattern) {
        if (ch == '/') {
            if (!current.empty()) {
                segments.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        segments.push_back(current);
    }
    return segments;
}

void append_replacement_char(std::string& out) {
    out.push_back(static_cast<char>(0xEF));
    out.push_back(static_cast<char>(0xBF));
    out.push_back(static_cast<char>(0xBD));
}

bool is_cont(unsigned char value) {
    return (value & 0xC0) == 0x80;
}

bool read_text_with_range(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line,
    std::vector<std::string>* lines_out) {

    content.clear();
    lines_read = 0;
    actual_end_line = 0;

    if (path.empty()) {
        return false;
    }
    if (start_line <= 0) {
        start_line = 1;
    }

    try {
        std::ifstream file = open_ifstream_unicode(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        unsigned char bom[3] = {0};
        file.read(reinterpret_cast<char*>(bom), 3);
        const std::streamsize got = file.gcount();
        if (!(got == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
            file.clear();
            file.seekg(0, std::ios::beg);
        }

        std::string line;
        std::vector<std::string> lines;
        if (lines_out) {
            lines_out->clear();
        }
        if (end_line > 0) {
            lines.reserve(std::max(0, end_line - start_line + 1));
        } else {
            lines.reserve(256);
        }

        int current_line = 1;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (current_line >= start_line) {
                if (end_line > 0 && current_line > end_line) {
                    break;
                }
                lines.emplace_back(std::move(line));
            }
            ++current_line;
        }

        for (std::size_t index = 0; index < lines.size(); ++index) {
            if (index) {
                content.push_back('\n');
            }
            content += lines[index];
        }

        if (lines_out) {
            *lines_out = lines;
        }

        lines_read = static_cast<int>(lines.size());
        actual_end_line = (lines_read > 0) ? (start_line + lines_read - 1) : (start_line - 1);
        return true;
    } catch (...) {
        content.clear();
        lines_read = 0;
        actual_end_line = 0;
        return false;
    }
}

} // namespace builtin_tools::internal
