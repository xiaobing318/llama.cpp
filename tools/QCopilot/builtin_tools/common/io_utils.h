#pragma once

#include "json.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

namespace builtin_tools::common {

using json = nlohmann::ordered_json;

bool read_file_content(const std::string& path, std::string& content);
bool write_file_content(const std::string& path, const std::string& content);
bool append_file_content(const std::string& path, const std::string& content);
std::ifstream open_ifstream_unicode(const std::string& path, std::ios::openmode mode = std::ios::in);

std::vector<json> search_in_file_regex(
    const std::filesystem::path& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches);

bool is_likely_binary(const std::filesystem::path& filepath, std::size_t probe = 4096);
bool is_likely_binary_string(const std::string& buffer);

} // namespace builtin_tools::common
