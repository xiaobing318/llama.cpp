#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace builtin_tools::common {

std::string get_current_timestamp();
int64_t get_current_time_ms();
std::string format_timestamp(const std::chrono::system_clock::time_point& tp);

} // namespace builtin_tools::common
