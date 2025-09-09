#pragma once

#include <filesystem>
#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include "json.hpp"

namespace qctest {
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;

inline fs::path make_temp_dir(const std::string &prefix = "qcopilot_test_") {
    auto base = fs::temp_directory_path();
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    fs::path dir = base / (prefix + std::to_string((long long) now));
    fs::create_directories(dir);
    return dir;
}

inline void write_binary(const fs::path &p, const std::string &bytes) {
    std::ofstream ofs(p, std::ios::binary);
    ofs.write(bytes.data(), (std::streamsize) bytes.size());
}

struct Test {
    int fails = 0;
    void check(bool cond, const std::string &msg) {
        if (!cond) {
            ++fails;
            std::cerr << "[FAIL] " << msg << std::endl;
        }
    }
    int finish() const {
        if (fails == 0) {
            std::cout << "[OK] all checks passed" << std::endl;
            return 0;
        }
        std::cerr << fails << " check(s) failed" << std::endl;
        return 1;
    }
};

// Simple JSON schema assertion helpers (lightweight, no external deps)
inline const char* type_name(const ordered_json &j) {
    if (j.is_null()) return "null";
    if (j.is_boolean()) return "boolean";
    if (j.is_number_integer()) return "integer";
    if (j.is_number()) return "number";
    if (j.is_string()) return "string";
    if (j.is_array()) return "array";
    if (j.is_object()) return "object";
    return "unknown";
}

inline bool has_type(const ordered_json &j, const std::string &t) {
    if (t == "null") return j.is_null();
    if (t == "boolean") return j.is_boolean();
    if (t == "integer") return j.is_number_integer();
    if (t == "number") return j.is_number();
    if (t == "string") return j.is_string();
    if (t == "array") return j.is_array();
    if (t == "object") return j.is_object();
    return false;
}

// Validate required keys and types; returns true if all checks pass
inline bool expect_json_schema(
    const ordered_json &j,
    const std::initializer_list<std::pair<const char*, const char*>> &required,
    Test &T,
    const std::string &ctx = "json") {
    bool ok = true;
    if (!j.is_object()) {
        T.check(false, ctx + ": not an object");
        return false;
    }
    for (auto &kv : required) {
        const char* key = kv.first;
        const char* ty  = kv.second;
        if (!j.contains(key)) {
            T.check(false, ctx + ": missing key '" + key + "'");
            ok = false; continue;
        }
        if (!has_type(j.at(key), ty)) {
            T.check(false, ctx + ": key '" + key + "' has type '" + type_name(j.at(key)) + "' but expected '" + ty + "'");
            ok = false;
        }
    }
    return ok;
}
}
