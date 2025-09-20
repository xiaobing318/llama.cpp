#include "tool_registry.h"

#include "builtin_tools/fileTools/read_text_lines.h"
#include "builtin_tools/fileTools/validate_utf8_file.h"
#include "builtin_tools/fileTools/write_text_file.h"
#include "builtin_tools/mathTools/calculate.h"
#include "builtin_tools/systemTools/glob.h"
#include "builtin_tools/systemTools/grep.h"
#include "builtin_tools/systemTools/list_directory.h"
#include "builtin_tools/systemTools/path_stat.h"
#include "builtin_tools/timeTools/get_current_time.h"

namespace BuiltinTools {

const std::vector<ToolRegistryEntry>& getToolRegistry() {
    static const std::vector<ToolRegistryEntry> registry = {
        {"get_current_time",   TimeTools::get_current_time_definition,   TimeTools::run_get_current_time},
        {"calculate",          MathTools::get_calculate_definition,      MathTools::run_calculate},
        {"read_text_lines",    FileTools::get_read_text_lines_definition, FileTools::run_read_text_lines},
        {"write_text_file",    FileTools::get_write_text_file_definition, FileTools::run_write_text_file},
        {"validate_utf8_file", FileTools::get_validate_utf8_file_definition, FileTools::run_validate_utf8_file},
        {"list_directory",     SystemTools::get_list_directory_definition, SystemTools::run_list_directory},
        {"path_stat",          SystemTools::get_path_stat_definition,      SystemTools::run_path_stat},
        {"grep",               SystemTools::get_grep_definition,           SystemTools::run_grep},
        {"glob",               SystemTools::get_glob_definition,           SystemTools::run_glob},
    };
    return registry;
}

} // namespace BuiltinTools
