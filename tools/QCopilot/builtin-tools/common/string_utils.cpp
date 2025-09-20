#include "string_utils.h"

#include "common_utils_internal.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace legacy_internal = BuiltinTools::Utils::Internal;

namespace builtin_tools::common {

std::string sanitize_string_for_json(const std::string& input) {
    if (input.empty()) {
        return input;
    }

    std::string out;
    out.reserve(input.size());

    const unsigned char* data = reinterpret_cast<const unsigned char*>(input.data());
    size_t i = 0;
    const size_t n = input.size();

    auto push_ascii = [&](unsigned char c) {
        if (c < 0x20) {
            if (c == '\n' || c == '\r' || c == '\t') {
                out.push_back(static_cast<char>(c));
            } else {
                out.push_back(' ');
            }
        } else if (c == 0x7F) {
            out.push_back(' ');
        } else {
            out.push_back(static_cast<char>(c));
        }
    };

    auto need_cont = [&](size_t k) {
        return i + k < n && (data[i + k] & 0xC0) == 0x80;
    };

    while (i < n) {
        unsigned char c = data[i];

        if (c <= 0x7F) {
            push_ascii(c);
            ++i;
            continue;
        }

        if (c >= 0xC2 && c <= 0xDF && need_cont(1)) {
            out.push_back(static_cast<char>(data[i++]));
            out.push_back(static_cast<char>(data[i++]));
            continue;
        }

        if (c == 0xE0 && need_cont(1) && need_cont(2) && data[i + 1] >= 0xA0 && data[i + 1] <= 0xBF) {
            out.append(reinterpret_cast<const char*>(data + i), 3);
            i += 3;
            continue;
        }
        if (c >= 0xE1 && c <= 0xEC && need_cont(1) && need_cont(2)) {
            out.append(reinterpret_cast<const char*>(data + i), 3);
            i += 3;
            continue;
        }
        if (c == 0xED && need_cont(1) && need_cont(2) && data[i + 1] >= 0x80 && data[i + 1] <= 0x9F) {
            out.append(reinterpret_cast<const char*>(data + i), 3);
            i += 3;
            continue;
        }
        if (c >= 0xEE && c <= 0xEF && need_cont(1) && need_cont(2)) {
            out.append(reinterpret_cast<const char*>(data + i), 3);
            i += 3;
            continue;
        }

        if (c == 0xF0 && need_cont(1) && need_cont(2) && need_cont(3) && data[i + 1] >= 0x90 && data[i + 1] <= 0xBF) {
            out.append(reinterpret_cast<const char*>(data + i), 4);
            i += 4;
            continue;
        }
        if (c >= 0xF1 && c <= 0xF3 && need_cont(1) && need_cont(2) && need_cont(3)) {
            out.append(reinterpret_cast<const char*>(data + i), 4);
            i += 4;
            continue;
        }
        if (c == 0xF4 && need_cont(1) && need_cont(2) && need_cont(3) && data[i + 1] >= 0x80 && data[i + 1] <= 0x8F) {
            out.append(reinterpret_cast<const char*>(data + i), 4);
            i += 4;
            continue;
        }

        legacy_internal::append_replacement_char(out);
        ++i;
    }

    return out;
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> tokens;
    tokens.reserve(std::count(str.begin(), str.end(), delimiter) + 1);

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.emplace_back(std::move(token));
    }
    return tokens;
}

std::string trim_string(const std::string& str) {
    const size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return {};
    }
    const size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) {
        return {};
    }
    if (strings.size() == 1) {
        return strings.front();
    }

    size_t total_size = 0;
    for (const auto& item : strings) {
        total_size += item.size();
    }
    total_size += delimiter.size() * (strings.size() - 1);

    std::string result;
    result.reserve(total_size);

    result = strings.front();
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter;
        result += strings[i];
    }
    return result;
}

std::string format_file_size(uintmax_t size_bytes) {
    static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double size = static_cast<double>(size_bytes);
    int unit = 0;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << size << ' ' << units[unit];
    return ss.str();
}

} // namespace builtin_tools::common
