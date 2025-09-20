#include "filesystem_utils.h"

#include "common_utils_internal.h"
#include "../../qcopilot_utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace builtin_tools::common {
namespace {

namespace fs = std::filesystem;

namespace legacy_internal = BuiltinTools::Utils::Internal;

bool looks_like_url(const std::string& value) {
    static const std::regex k_url_regex(R"(^[A-Za-z][A-Za-z0-9+\-.]*://)");
    return std::regex_search(value, k_url_regex);
}

bool contains_nul_or_control(const std::string& value) {
    for (unsigned char ch : value) {
        if (ch == 0 || (ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r')) {
            return true;
        }
    }
    return false;
}

#ifdef _WIN32
bool is_windows_reserved_device(const std::wstring& name) {
    if (name.empty()) {
        return false;
    }

    auto to_upper = [](wchar_t c) { return static_cast<wchar_t>(std::toupper(c)); };
    std::wstring upper;
    upper.reserve(name.size());
    for (auto c : name) {
        upper.push_back(to_upper(c));
    }

    while (!upper.empty() && (upper.back() == L' ' || upper.back() == L'.')) {
        upper.pop_back();
    }
    if (upper.empty()) {
        return false;
    }

    auto starts_with = [&](const std::wstring& prefix) {
        return upper.size() >= prefix.size() &&
               std::equal(prefix.begin(), prefix.end(), upper.begin());
    };

    static const std::wstring base_devices[] = { L"CON", L"PRN", L"AUX", L"NUL" };
    for (const auto& device : base_devices) {
        if (starts_with(device) && (upper.size() == device.size() || upper[device.size()] == L'.')) {
            return true;
        }
    }

    if (upper.size() >= 4) {
        if ((upper.rfind(L"COM", 0) == 0 || upper.rfind(L"LPT", 0) == 0) &&
            upper[3] >= L'1' && upper[3] <= L'9' &&
            (upper.size() == 4 || upper[4] == L'.')) {
            return true;
        }
    }

    return false;
}

bool windows_component_invalid(const std::wstring& component) {
    if (component.empty()) {
        return false;
    }

    if (component.back() == L' ' || component.back() == L'.') {
        return true;
    }

    for (wchar_t wc : component) {
        if (wc < 0x20) {
            return true;
        }
        switch (wc) {
            case L'<':
            case L'>':
            case L':':
            case L'"':
            case L'|':
            case L'?':
            case L'*':
                return true;
            default:
                break;
        }
    }

    return is_windows_reserved_device(component);
}
#endif // _WIN32

bool contains_traversal(const fs::path& normalized) {
    for (const auto& part : normalized) {
        if (part == "..") {
            return true;
        }
    }
    return false;
}

bool check_windows_components(const fs::path& normalized, std::string& error_message) {
#ifdef _WIN32
    for (const auto& part : normalized) {
        if (part.native().empty()) {
            continue;
        }
        const std::wstring component = part.native();
        if (component == L"\\" || component == L"/") {
            continue;
        }
        if (windows_component_invalid(component)) {
            error_message = "Invalid Windows path component (reserved or contains forbidden characters)";
            return false;
        }
    }
#endif
    return true;
}

bool normalize_path(const std::string& input, fs::path& normalized, std::string& error_message) {
    if (input.empty()) {
        error_message = "Path is required";
        LOG_WRN("validate path: empty input");
        return false;
    }

    if (looks_like_url(input)) {
        error_message = "URL is not allowed; local filesystem paths only";
        LOG_WRN("validate path: URL detected: %s", input.c_str());
        return false;
    }

    if (contains_nul_or_control(input)) {
        error_message = "Path contains NUL or control characters";
        LOG_WRN("validate path: control character: %s", input.c_str());
        return false;
    }

    if (input.size() > 4096) {
        error_message = "Path too long (maximum 4096 bytes in UTF-8)";
        LOG_WRN("validate path: over length (len=%zu): %s", input.size(), input.c_str());
        return false;
    }

    try {
        const fs::path raw = utf8_to_path(input);
        normalized = raw.lexically_normal();
    } catch (const std::exception& e) {
        error_message = std::string("Invalid path: ") + e.what();
        LOG_WRN("validate path: exception for '%s': %s", input.c_str(), e.what());
        return false;
    }

    if (contains_traversal(normalized)) {
        error_message = "Path traversal not allowed";
        LOG_WRN("validate path: traversal after normalize: %s", input.c_str());
        return false;
    }

    if (!check_windows_components(normalized, error_message)) {
        LOG_WRN("validate path: windows invalid component: %s", input.c_str());
        return false;
    }

    return true;
}

fs::path resolve_parent(const fs::path& normalized) {
    std::error_code ec;
    fs::path parent = normalized.parent_path();
    if (parent.empty()) {
        fs::path cwd = fs::current_path(ec);
        return ec ? fs::path{} : cwd;
    }
    if (parent.is_absolute()) {
        return parent;
    }
    fs::path cwd = fs::current_path(ec);
    if (ec) {
        return parent;
    }
    return cwd / parent;
}

} // namespace

namespace fs = std::filesystem;

std::filesystem::path utf8_to_path(const std::string& input) {
    return fs::u8path(input);
}

std::string path_to_utf8_string(const std::filesystem::path& path) {
#if defined(__cpp_lib_char8_t)
    auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
#else
    return path.u8string();
#endif
}

bool validate_existing_path(const std::string& path, std::string& error_message) {
    fs::path normalized;
    if (!normalize_path(path, normalized, error_message)) {
        return false;
    }

    std::error_code ec;
    const bool exists = fs::exists(normalized, ec);
    if (ec) {
        error_message = std::string("Filesystem error: ") + ec.message();
        LOG_WRN("validate_existing_path: exists() error for '%s': %s", path.c_str(), ec.message().c_str());
        return false;
    }
    if (!exists) {
        error_message = "Path does not exist";
        LOG_WRN("validate_existing_path: not exists: %s", path.c_str());
        return false;
    }

    return true;
}

bool prepare_writable_path(const std::string& path, std::string& error_message) {
    fs::path normalized;
    if (!normalize_path(path, normalized, error_message)) {
        return false;
    }

    std::error_code ec;
    const fs::path parent = resolve_parent(normalized);

    bool parent_exists = fs::exists(parent, ec);
    if (ec) {
        error_message = std::string("Filesystem error: ") + ec.message();
        LOG_WRN("prepare_writable_path: exists() error for parent '%s': %s",
                path_to_utf8_string(parent).c_str(), ec.message().c_str());
        return false;
    }
    if (!parent_exists) {
        error_message = "Parent directory does not exist";
        LOG_WRN("prepare_writable_path: parent not exists: %s", path_to_utf8_string(parent).c_str());
        return false;
    }

    ec.clear();
    bool parent_is_dir = fs::is_directory(parent, ec);
    if (ec) {
        error_message = std::string("Filesystem error: ") + ec.message();
        LOG_WRN("prepare_writable_path: is_directory error for parent '%s': %s",
                path_to_utf8_string(parent).c_str(), ec.message().c_str());
        return false;
    }
    if (!parent_is_dir) {
        error_message = "Parent path is not a directory";
        LOG_WRN("prepare_writable_path: parent not directory: %s", path_to_utf8_string(parent).c_str());
        return false;
    }

    ec.clear();
    bool target_exists = fs::exists(normalized, ec);
    if (ec) {
        error_message = std::string("Filesystem error: ") + ec.message();
        LOG_WRN("prepare_writable_path: exists() error for '%s': %s", path.c_str(), ec.message().c_str());
        return false;
    }

    if (target_exists) {
        ec.clear();
        bool target_is_dir = fs::is_directory(normalized, ec);
        if (ec) {
            error_message = std::string("Filesystem error: ") + ec.message();
            LOG_WRN("prepare_writable_path: is_directory error for '%s': %s", path.c_str(), ec.message().c_str());
            return false;
        }
        if (target_is_dir) {
            error_message = "Target path is an existing directory";
            LOG_WRN("prepare_writable_path: target is directory: %s", path.c_str());
            return false;
        }
    }

    return true;
}

bool file_exists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return fs::exists(utf8_to_path(path));
    } catch (const fs::filesystem_error& e) {
        LOG_WRN("file_exists: filesystem error checking '%s': %s", path.c_str(), e.what());
        return false;
    }
}

bool is_regular_readable_file(const std::string& path, std::string& error_message) {
    try {
        const fs::path target = utf8_to_path(path);
        if (!fs::exists(target)) {
            error_message = "Path does not exist";
            return false;
        }
        if (!fs::is_regular_file(target)) {
            error_message = "Path is not a regular file";
            return false;
        }

        std::ifstream stream(target, std::ios::binary);
        if (!stream) {
            error_message = "Failed to open file for reading";
            return false;
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        error_message = e.what();
        return false;
    }
}

std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> entries;
    try {
        const fs::path dir = utf8_to_path(path);
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            return entries;
        }

        auto options = fs::directory_options::skip_permission_denied;
        for (const auto& entry : fs::directory_iterator(dir, options)) {
            entries.push_back(path_to_utf8_string(entry.path().filename()));
        }
        std::sort(entries.begin(), entries.end());
    } catch (const fs::filesystem_error& e) {
        LOG_WRN("list_directory: filesystem error for '%s': %s", path.c_str(), e.what());
        entries.clear();
    }
    return entries;
}

std::vector<std::filesystem::path> glob_paths(
    const std::filesystem::path& base_dir,
    const std::string& pattern,
    bool include_directories,
    bool follow_symlinks,
    bool case_sensitive) {

    std::vector<std::filesystem::path> results;
    try {
        std::vector<std::string> segments = legacy_internal::split_pattern_segments(pattern);
        if (segments.empty()) {
            return results;
        }

        std::vector<fs::path> frontier = { fs::weakly_canonical(base_dir) };

        for (std::size_t idx = 0; idx < segments.size(); ++idx) {
            const std::string& segment = segments[idx];
            const bool last_segment = (idx + 1 == segments.size());

            if (segment == "**") {
                std::vector<fs::path> expanded;
                for (const auto& root : frontier) {
                    if (!fs::exists(root) || !fs::is_directory(root)) {
                        continue;
                    }
                    expanded.push_back(root);

                    fs::directory_options options = fs::directory_options::skip_permission_denied;
                    if (follow_symlinks) {
                        options |= fs::directory_options::follow_directory_symlink;
                    }

                    for (auto it = fs::recursive_directory_iterator(root, options);
                         it != fs::recursive_directory_iterator(); ++it) {
                        if (it->is_directory()) {
                            expanded.push_back(it->path());
                        }
                    }
                }
                frontier.swap(expanded);
                continue;
            }

            std::vector<fs::path> next;
            for (const auto& dir : frontier) {
                if (!fs::exists(dir) || !fs::is_directory(dir)) {
                    continue;
                }

                fs::directory_options options = fs::directory_options::skip_permission_denied;
                if (follow_symlinks) {
                    options |= fs::directory_options::follow_directory_symlink;
                }

                for (auto& entry : fs::directory_iterator(dir, options)) {
                    const std::string name = path_to_utf8_string(entry.path().filename());
                    if (!legacy_internal::glob_segment_match(name, segment, case_sensitive)) {
                        continue;
                    }

                    if (last_segment) {
                        if (include_directories || entry.is_regular_file()) {
                            results.push_back(entry.path());
                        }
                    } else {
                        if (entry.is_directory()) {
                            next.push_back(entry.path());
                        }
                    }
                }
            }
            frontier.swap(next);
        }

        std::sort(results.begin(), results.end());
        results.erase(std::unique(results.begin(), results.end()), results.end());
    } catch (const fs::filesystem_error& e) {
        LOG_WRN("glob_paths: filesystem error for '%s': %s", base_dir.string().c_str(), e.what());
        results.clear();
    }
    return results;
}

} // namespace builtin_tools::common
