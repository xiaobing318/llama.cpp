#include "encoding_utils.h"

#include "common_utils_internal.h"
#include "filesystem_utils.h"
#include "io_utils.h"

namespace builtin_tools::common {

namespace legacy_internal = BuiltinTools::Utils::Internal;

bool is_valid_utf8_string(const std::string& value) {
    const unsigned char* data = reinterpret_cast<const unsigned char*>(value.data());
    size_t i = 0;
    const size_t n = value.size();

    while (i < n) {
        const unsigned char c = data[i];

        if (c <= 0x7F) {
            ++i;
            continue;
        }

        if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 >= n || !legacy_internal::is_cont(data[i + 1])) {
                return false;
            }
            i += 2;
            continue;
        }

        if (c == 0xE0) {
            if (i + 2 >= n) {
                return false;
            }
            const unsigned char b1 = data[i + 1];
            const unsigned char b2 = data[i + 2];
            if (!(b1 >= 0xA0 && b1 <= 0xBF) || !legacy_internal::is_cont(b2)) {
                return false;
            }
            i += 3;
            continue;
        }
        if (c >= 0xE1 && c <= 0xEC) {
            if (i + 2 >= n || !legacy_internal::is_cont(data[i + 1]) || !legacy_internal::is_cont(data[i + 2])) {
                return false;
            }
            i += 3;
            continue;
        }
        if (c == 0xED) {
            if (i + 2 >= n) {
                return false;
            }
            const unsigned char b1 = data[i + 1];
            const unsigned char b2 = data[i + 2];
            if (!(b1 >= 0x80 && b1 <= 0x9F) || !legacy_internal::is_cont(b2)) {
                return false;
            }
            i += 3;
            continue;
        }
        if (c >= 0xEE && c <= 0xEF) {
            if (i + 2 >= n || !legacy_internal::is_cont(data[i + 1]) || !legacy_internal::is_cont(data[i + 2])) {
                return false;
            }
            i += 3;
            continue;
        }

        if (c == 0xF0) {
            if (i + 3 >= n) {
                return false;
            }
            const unsigned char b1 = data[i + 1];
            const unsigned char b2 = data[i + 2];
            const unsigned char b3 = data[i + 3];
            if (!(b1 >= 0x90 && b1 <= 0xBF) || !legacy_internal::is_cont(b2) || !legacy_internal::is_cont(b3)) {
                return false;
            }
            i += 4;
            continue;
        }
        if (c >= 0xF1 && c <= 0xF3) {
            if (i + 3 >= n || !legacy_internal::is_cont(data[i + 1]) || !legacy_internal::is_cont(data[i + 2]) ||
                !legacy_internal::is_cont(data[i + 3])) {
                return false;
            }
            i += 4;
            continue;
        }
        if (c == 0xF4) {
            if (i + 3 >= n) {
                return false;
            }
            const unsigned char b1 = data[i + 1];
            const unsigned char b2 = data[i + 2];
            const unsigned char b3 = data[i + 3];
            if (!(b1 >= 0x80 && b1 <= 0x8F) || !legacy_internal::is_cont(b2) || !legacy_internal::is_cont(b3)) {
                return false;
            }
            i += 4;
            continue;
        }

        return false;
    }

    return true;
}

bool is_valid_utf8_file(const std::string& path) {
    if (!file_exists(path)) {
        return false;
    }

    std::string content;
    if (!read_file_content(path, content)) {
        return false;
    }

    return is_valid_utf8_string(content);
}

} // namespace builtin_tools::common
