#include "get_current_time.h"
#include "../common/common_utils.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace BuiltinTools {
namespace TimeTools {

ToolDefinition getGetCurrentTimeDefinition() {
    return {
        "get_current_time",
        {
            {"type", "function"},
            {"function", {
                {"name", "get_current_time"},
                {"description", "Retrieves the current system time in various formats for timestamping, logging, and temporal data processing. Essential for applications requiring precise time tracking, data synchronization, and audit trails. Use cases include: 1) Adding creation timestamps to log entries and data records 2) Generating time-based unique identifiers for files and database entries 3) Calculating elapsed time intervals and performance metrics 4) Synchronizing distributed systems and coordinating batch processing jobs 5) Creating temporal metadata for GIS datasets and scientific measurements. The tool supports multiple output formats to accommodate different systems and use cases."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"format", {{"type", "string"}, {"description", "Time format specification: 'ISO8601' for standard international format (YYYY-MM-DDTHH:MM:SS, ideal for databases and APIs), 'unix' for Unix timestamp (seconds since epoch, optimal for calculations and system interoperability), or 'default' for human-readable format (YYYY-MM-DD HH:MM:SS, suitable for user interfaces and reports). Defaults to ISO8601."}}},
                        {"timezone", {{"type", "string"}, {"description", "Timezone specification: 'local' for system local timezone (appropriate for user-facing applications and local file processing), or 'UTC' for Coordinated Universal Time (recommended for distributed systems, logging, and international data exchange). Defaults to local timezone."}}}
                    }}
                }}
            }}
        }
    };
}

json executeGetCurrentTime(const json& args) {
    // 提取参数，设置默认值
    std::string format = args.value("format", "ISO8601");
    std::string timezone = args.value("timezone", "local");
    // 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    // 根据时区参数选择合适的时间转换函数
    std::tm* time_info = nullptr;
    if (timezone == "UTC") {
        time_info = std::gmtime(&time_t);
    } else {
        time_info = std::localtime(&time_t);
    }

    // 错误处理：检查时间转换是否成功
    if (!time_info) {
        LOG_ERR("get_current_time: Failed to convert system time for timezone '%s'", timezone.c_str());
        return BuiltinTools::Utils::createErrorResponse("Failed to convert system time");
    }

    std::stringstream ss;
    if (format == "ISO8601") {
        ss << std::put_time(time_info, "%Y-%m-%dT%H:%M:%S");
        // 为 UTC 时间添加 Z 后缀，符合 ISO8601 标准
        if (timezone == "UTC") {
            ss << "Z";
        }
    } else if (format == "unix") {
        ss << time_t;
    } else {
        // default 格式
        ss << std::put_time(time_info, "%Y-%m-%d %H:%M:%S");
    }

    json result = BuiltinTools::Utils::createSuccessResponse();
    result["time"] = ss.str();
    result["format"] = format;
    result["timezone"] = timezone;

    return result;
}

} // namespace TimeTools
} // namespace BuiltinTools
