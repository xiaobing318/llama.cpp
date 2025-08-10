#include "agent_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

void common_init() {
    // Set UTF-8 locale
    std::setlocale(LC_ALL, "");

#ifdef _WIN32
    // Enable UTF-8 console output on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // 强制 C++ 输出流使用 UTF-8 locale
    std::setlocale(LC_ALL, ".UTF-8");
#endif

    // Log initialization
    LOG_INF("Agent utilities initialized\n");
}

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int64_t get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    std::stringstream ss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) ss << delimiter;
        ss << strings[i];
    }
    return ss.str();
}

bool file_exists(const std::string& path) {
    return fs::exists(path);
}

bool read_file_content(const std::string& path, std::string& content) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        content.resize(size);
        file.read(&content[0], size);
        file.close();

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to read file %s: %s\n", path.c_str(), e.what());
        return false;
    }
}

bool write_file_content(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.c_str(), content.size());
        file.close();

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to write file %s: %s\n", path.c_str(), e.what());
        return false;
    }
}

std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> files;

    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            return files;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            files.push_back(entry.path().filename().string());
        }
    } catch (const std::exception& e) {
        LOG_ERR("Failed to list directory %s: %s\n", path.c_str(), e.what());
    }

    return files;
}

std::string execute_command(const std::string& command) {
    std::string result;

#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        return "ERROR: Failed to execute command";
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}

bool is_process_running(int pid) {
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == NULL) {
        return false;
    }

    DWORD exitCode;
    GetExitCodeProcess(process, &exitCode);
    CloseHandle(process);

    return exitCode == STILL_ACTIVE;
#else
    return kill(pid, 0) == 0;
#endif
}

json safe_parse_json(const std::string& str) {
    try {
        return json::parse(str);
    } catch (const std::exception& e) {
        LOG_ERR("Failed to parse JSON: %s\n", e.what());
        return json();
    }
}

std::string format_json(const json& j) {
    return j.dump(2);
}

bool validate_tool_name(const std::string& name) {
    if (name.empty()) return false;

    // Check if name contains only alphanumeric characters and underscores
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalnum(c) || c == '_';
    });
}

bool validate_arguments(const json& args, const json& schema) {
    // Simple validation - check required fields
    if (!schema.contains("required")) {
        return true;
    }

    for (const auto& required : schema["required"]) {
        std::string field = required.get<std::string>();
        if (!args.contains(field)) {
            LOG_ERR("Missing required field: %s\n", field.c_str());
            return false;
        }
    }

    return true;
}
