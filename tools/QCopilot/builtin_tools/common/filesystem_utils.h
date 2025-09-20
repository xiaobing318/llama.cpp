#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace builtin_tools::common {

std::filesystem::path utf8_to_path(const std::string& input);
std::string path_to_utf8_string(const std::filesystem::path& path);

bool validate_existing_path(const std::string& path, std::string& error_message);
bool prepare_writable_path(const std::string& path, std::string& error_message);

bool file_exists(const std::string& path);
bool is_regular_readable_file(const std::string& path, std::string& error_message);
std::vector<std::string> list_directory(const std::string& path);

std::vector<std::filesystem::path> glob_paths(
    const std::filesystem::path& base_dir,
    const std::string& pattern,
    bool include_directories = false,
    bool follow_symlinks = false,
    bool case_sensitive =
#ifdef _WIN32
        false
#else
        true
#endif
);

} // namespace builtin_tools::common
