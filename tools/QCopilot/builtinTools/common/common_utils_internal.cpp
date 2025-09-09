#include "common_utils_internal.h"
#include "common_utils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

namespace BuiltinTools {
namespace Utils {
namespace Internal {

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

bool matchCharClass(char c, const std::string& cls, bool case_sensitive){
    char cc = c;
    if (!case_sensitive) cc = to_lower_unsafe(cc);
    bool negate = false;
    std::size_t i = 0;
    if (i < cls.size() && cls[i] == '!') { negate = true; ++i; }

    bool ok = false;
    while (i < cls.size()) {
        char first = cls[i++];
        if (!case_sensitive) first = to_lower_unsafe(first);
        if (i + 1 < cls.size() && cls[i] == '-') {
            ++i;
            char last = cls[i++];
            if (!case_sensitive) last = to_lower_unsafe(last);
            if (first <= cc && cc <= last) ok = true;
        } else {
            if (first == cc) ok = true;
        }
    }
    return negate ? !ok : ok;
}

bool globSegmentMatch(const std::string& text, const std::string& pat, bool case_sensitive) {
    const std::string t = case_sensitive ? text : normalize_case(text, false);
    const std::string p = case_sensitive ? pat  : normalize_case(pat,  false);

    std::size_t ti = 0, pi = 0;
    std::size_t star_pi = std::string::npos, star_ti = 0;

    while (ti < t.size()) {
        if (pi < p.size()) {
            if (p[pi] == '?') { ++pi; ++ti; continue; }
            if (p[pi] == '\\') {
                ++pi;
                if (pi < p.size() && p[pi] == t[ti]) { ++pi; ++ti; continue; }
            } else if (p[pi] == '[') {
                std::size_t end = p.find(']', pi + 1);
                if (end == std::string::npos) return false;
                std::string cls = p.substr(pi + 1, end - (pi + 1));
                char tc = text[ti];
                if (!case_sensitive) tc = to_lower_unsafe(tc);
                if (!matchCharClass(tc, cls, case_sensitive)) {
                    if (star_pi != std::string::npos) { ti = ++star_ti; pi = star_pi + 1; continue; }
                    return false;
                }
                pi = end + 1; ++ti; continue;
            } else if (p[pi] == '*') {
                star_pi = pi;
                star_ti = ti;
                ++pi; continue;
            } else if (p[pi] == t[ti]) {
                ++pi; ++ti; continue;
            }
        }
        if (star_pi != std::string::npos) { ti = ++star_ti; pi = star_pi + 1; continue; }
        return false;
    }
    while (pi < p.size() && p[pi] == '*') ++pi;
    return pi == p.size();
}

std::vector<std::string> splitPatternSegments(std::string pat) {
    pat = slashify(std::move(pat));
    std::vector<std::string> segs;
    std::string cur;
    for (char ch : pat) {
        if (ch == '/') {
            if (!cur.empty()) segs.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    if (!cur.empty()) segs.push_back(cur);
    return segs;
}

void append_replacement_char(std::string& out) {
    out.push_back(static_cast<char>(0xEF));
    out.push_back(static_cast<char>(0xBF));
    out.push_back(static_cast<char>(0xBD));
}

bool is_cont(unsigned char x) { return (x & 0xC0) == 0x80; }

bool readTextFileWithRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line) {

    content.clear();
    lines_read = 0;
    actual_end_line = 0;

    if (path.empty()) return false;
    if (start_line <= 0) start_line = 1;

    try {
        std::ifstream file = BuiltinTools::Utils::open_ifstream_unicode(path, std::ios::binary);
        if (!file.is_open()) return false;

        unsigned char bom[3] = {0};
        file.read(reinterpret_cast<char*>(bom), 3);
        std::streamsize got = file.gcount();
        if (!(got == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
            file.clear();
            file.seekg(0, std::ios::beg);
        }

        std::string line;
        std::vector<std::string> lines;
        lines.reserve((end_line > 0) ? std::max(0, end_line - start_line + 1) : 256);

        int current_line = 1;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (current_line >= start_line) {
                if (end_line > 0 && current_line > end_line) break;
                lines.emplace_back(std::move(line));
            }
            ++current_line;
        }

        for (size_t i = 0; i < lines.size(); ++i) {
            if (i) content.push_back('\n');
            content += lines[i];
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

} // namespace Internal
} // namespace Utils
} // namespace BuiltinTools
