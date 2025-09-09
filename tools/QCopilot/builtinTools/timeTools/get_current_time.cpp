#include "get_current_time.h"
#include "../common/common_utils.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <set>
#include <cerrno>
#include <cstring>
#include <algorithm> // std::transform
#include <cmath>     // std::abs

namespace BuiltinTools {
namespace TimeTools {

// 内部辅助函数：线程安全地将 time_t 分解为 tm
static bool to_tm_safe(const std::time_t& tt, const std::string& tz, std::tm& out, std::string& err) {
    // Windows 和 POSIX 有不同的线程安全 API，因此使用条件编译
#if defined(_WIN32)
    errno_t rc = (tz == "UTC") ? gmtime_s(&out, &tt) : localtime_s(&out, &tt);
    if (rc != 0) {
        err = "to_tm_safe failed on Windows, rc=" + std::to_string(rc);
        return false;
    }
#else
    std::tm* res = (tz == "UTC") ? gmtime_r(&tt, &out) : localtime_r(&tt, &out);
    if (res == nullptr) {
        err = std::string("to_tm_safe failed on POSIX: ") + std::strerror(errno);
        return false;
    }
#endif
    return true;
}

// 内部辅助函数：计算本地与 UTC 的秒级偏移（可移植，不依赖 %z / timegm）
static int local_utc_offset_seconds(const std::time_t& tt) {
    std::tm tm_utc{};
    // Windows 和 POSIX 有不同的线程安全 API，因此使用条件编译
#if defined(_WIN32)
    if (gmtime_s(&tm_utc, &tt) != 0) return 0;
#else
    if (gmtime_r(&tt, &tm_utc) == nullptr) return 0;
#endif
    // 将“UTC 的分解时间”当作“本地时间”喂给 mktime，得到的 epoch 与真实 epoch 的差即为时区偏移
    std::tm tm_utc_as_local = tm_utc; // mktime 可能会规范化 tm
    std::time_t local_interpretation = std::mktime(&tm_utc_as_local);
    if (local_interpretation == (std::time_t)-1) return 0;
    return static_cast<int>(tt - local_interpretation); // 东八区返回正值（例如 28800）
}

// 内部辅助函数：以 ±HH:MM 形式追加时区偏移
static void append_tz_offset(std::ostream& os, int offset_seconds) {
    char sign = offset_seconds >= 0 ? '+' : '-';
    int abssec = std::abs(offset_seconds);
    int hh = abssec / 3600;
    int mm = (abssec % 3600) / 60;
    os << sign << std::setw(2) << std::setfill('0') << hh
       << ":" << std::setw(2) << std::setfill('0') << mm;
}

// 获取get_current_time工具的定义
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
                        {"format",   {{"type", "string"}, {"enum", {"ISO8601","unix","default"}}, {"default","ISO8601"}, {"description","Output format. 'ISO8601' (YYYY-MM-DDTHH:MM:SS[.mmm]Z or ±HH:MM), 'unix' (seconds since epoch), 'default' (YYYY-MM-DD HH:MM:SS)."}}},
                        {"timezone", {{"type", "string"}, {"enum", {"local","UTC"}}, {"default","local"}, {"description","Timezone to use. 'local' or 'UTC'."}}}
                    }}
                }}
            }}
        }
    };
}

// 执行get_current_time工具
json executeGetCurrentTime(const json& args) {
    // 1) 读取与校正参数 + 入参日志
    std::string format  = args.value("format",   "ISO8601");
    std::string timezone= args.value("timezone", "local");
    // 创建两个匿名函数用来对字符串进行大小写转换
    auto to_upper = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::toupper(c)); }); return s; };
    auto to_lower = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::tolower(c)); }); return s; };
    // 创建原始参数备份以便日志输出
    std::string format_orig = format;
    std::string tz_orig     = timezone;
    // 统一大小写
    format   = to_upper(format);
    timezone = (to_upper(timezone) == "UTC") ? "UTC" : "local";
    // 罗列支持的 format
    static const std::set<std::string> kFormats = {"ISO8601", "UNIX", "DEFAULT"};
    // 如果不支持，回退到 ISO8601
    bool format_fallback = false;
    if (!kFormats.count(format)) {
        // 终端给出警告日志
        LOG_WRN("get_current_time: unsupported format='%s', fallback to 'ISO8601'", format_orig.c_str());
        // 回退到 ISO8601
        format = "ISO8601";
        format_fallback = true;
    }

    // 2) 获取当前时间点
    const auto now   = std::chrono::system_clock::now();
    const auto secs  = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto msec  = std::chrono::duration_cast<std::chrono::milliseconds>(now - secs).count();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);

    // 3) 分解时间（线程安全 API）
    std::tm tm{};
    std::string err;
    if (!to_tm_safe(tt, timezone, tm, err)) {
        // 终端给出错误日志
        LOG_ERR("get_current_time: to_tm_safe failed (timezone='%s', time_t=%lld). detail=%s",
                timezone.c_str(), static_cast<long long>(tt), err.c_str());
        return BuiltinTools::Utils::createErrorResponse("Failed to convert system time");
    }

    // 4) 按格式输出
    std::stringstream ss;
    if (format == "UNIX") {
        // 只输出整秒，保持向后兼容
        ss << static_cast<long long>(tt);
    } else if (format == "ISO8601") {
        // 例：2025-09-09T10:23:45.123Z 或 2025-09-09T10:23:45.123+08:00
        ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setw(3) << std::setfill('0') << msec;
        if (timezone == "UTC") {
            ss << 'Z';
        } else {
            int offset = local_utc_offset_seconds(tt);
            append_tz_offset(ss, offset);
        }
    } else {
        // default：人类可读，但不含时区信息
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    }

    // 5) 结果 + 成功日志（debug）
    json result = BuiltinTools::Utils::createSuccessResponse();
    result["time"]     = ss.str();
    result["format"]   = (format == "UNIX" ? "unix" : (format == "ISO8601" ? "ISO8601" : "default"));
    result["timezone"] = timezone;
    if (format_fallback) {
        result["messages"] = json::array({"Unsupported format was requested; fell back to ISO8601"});
    }

    return result;
}

} // namespace TimeTools
} // namespace BuiltinTools
