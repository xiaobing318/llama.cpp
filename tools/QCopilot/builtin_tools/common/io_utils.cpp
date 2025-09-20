#include "io_utils.h"

#include "common_utils_internal.h"
#include "filesystem_utils.h"
#include "../../qcopilot_utils.h"

#include <algorithm>
#include <ios>
#include <new>
#include <regex>
#include <sstream>

namespace builtin_tools::common {

namespace fs = std::filesystem;
namespace legacy_internal = BuiltinTools::Utils::Internal;

bool read_file_content(const std::string& path, std::string& content) {
    if (path.empty()) {
        return false;
    }

    try {
        std::ifstream file = open_ifstream_unicode(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        const std::streampos file_size = file.tellg();
        if (file_size == std::streampos(-1)) {
            return false;
        }

        if (file_size == 0) {
            content.clear();
            return true;
        }

        const size_t size = static_cast<size_t>(file_size);
        static constexpr size_t k_max_file_size = 100 * 1024 * 1024; // 100 MB
        if (size > k_max_file_size) {
            LOG_WRN("read_file_content: file too large '%s': %zu bytes (max %zu)",
                    path.c_str(), size, k_max_file_size);
            return false;
        }

        file.seekg(0, std::ios::beg);
        if (file.fail()) {
            return false;
        }

        try {
            content.resize(size);
        } catch (const std::bad_alloc& e) {
            LOG_ERR("read_file_content: allocation failed for '%s': %s", path.c_str(), e.what());
            return false;
        }

        file.read(content.data(), static_cast<std::streamsize>(size));
        if (file.fail()) {
            content.clear();
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("read_file_content: exception reading '%s': %s", path.c_str(), e.what());
        return false;
    }
}

bool write_file_content(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (file.fail()) {
            file.close();
            return false;
        }

        file.close();
        if (file.fail()) {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("write_file_content: failed to write '%s': %s", path.c_str(), e.what());
        return false;
    }
}

bool append_file_content(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (file.fail()) {
            file.close();
            return false;
        }

        file.close();
        if (file.fail()) {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("append_file_content: failed to append '%s': %s", path.c_str(), e.what());
        return false;
    }
}

std::ifstream open_ifstream_unicode(const std::string& path, std::ios::openmode mode) {
    return std::ifstream(fs::u8path(path), mode);
}

std::vector<json> search_in_file_regex(
    const std::filesystem::path& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches) {

    std::vector<json> matches;
    if (max_matches <= 0) {
        return matches;
    }

    if (is_likely_binary(filepath)) {
        return matches;
    }

    std::ifstream stream(filepath);
    if (!stream) {
        return matches;
    }

    std::regex re;
    std::string needle = pattern;
    if (use_regex) {
        try {
            re = std::regex(pattern,
                            case_sensitive ? std::regex::ECMAScript
                                            : (std::regex::ECMAScript | std::regex::icase));
        } catch (const std::regex_error&) {
            return matches;
        }
    } else if (!case_sensitive) {
        needle = legacy_internal::normalize_case(needle, false);
    }

    std::string line;
    std::size_t line_number = 1;

    while (std::getline(stream, line)) {
        bool found = false;
        std::size_t pos_begin = std::string::npos;
        std::size_t pos_end = std::string::npos;

        if (use_regex) {
            std::smatch match;
            if (std::regex_search(line, match, re)) {
                found = true;
                pos_begin = static_cast<std::size_t>(match.position());
                pos_end = pos_begin + static_cast<std::size_t>(match.length());
            }
        } else {
            if (case_sensitive) {
                pos_begin = line.find(needle);
            } else {
                const std::string lower_line = legacy_internal::normalize_case(line, false);
                pos_begin = lower_line.find(needle);
            }
            found = (pos_begin != std::string::npos);
            if (found) {
                pos_end = pos_begin + needle.size();
            }
        }

        if (found) {
            json entry = {
                { "file",         path_to_utf8_string(filepath) },
                { "line_content", line },
                { "match_start",  static_cast<int>(pos_begin) },
                { "match_end",    static_cast<int>(pos_end) }
            };
            if (line_numbers) {
                entry["line_number"] = static_cast<int>(line_number);
            }

            matches.push_back(std::move(entry));
            ++total_matches;

            if (total_matches >= max_matches) {
                break;
            }
        }

        if (total_matches >= max_matches) {
            break;
        }

        ++line_number;
    }

    return matches;
}

bool is_likely_binary(const std::filesystem::path& filepath, std::size_t probe) {
    std::ifstream stream(filepath, std::ios::binary);
    if (!stream) {
        return false;
    }

    std::string buffer;
    buffer.resize(probe);
    stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const std::streamsize read_bytes = stream.gcount();
    for (std::streamsize i = 0; i < read_bytes; ++i) {
        const unsigned char c = static_cast<unsigned char>(buffer[static_cast<size_t>(i)]);
        if (c == 0) {
            return true;
        }
    }
    return false;
}

bool is_likely_binary_string(const std::string& buffer) {
    for (unsigned char c : buffer) {
        if (c == 0) {
            return true;
        }
    }
    return false;
}

} // namespace builtin_tools::common
