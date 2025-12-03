#include "tools-registry.h"

#include "builtin-tools/MathTools/MathTools-basic-math-calculator.h"

//#include "builtin-tools/fileTools/read_text_lines.h"
//#include "builtin-tools/fileTools/validate_utf8_file.h"
//#include "builtin-tools/fileTools/write_text_file.h"
//#include "builtin-tools/systemTools/glob.h"
//#include "builtin-tools/systemTools/grep.h"
//#include "builtin-tools/systemTools/list_directory.h"
//#include "builtin-tools/systemTools/path_stat.h"
//#include "builtin-tools/timeTools/get_current_time.h"

namespace BuiltinTools {

const std::vector<ToolRegistryEntry>& getToolRegistry() {
    static const std::vector<ToolRegistryEntry> registry = {
        {"basic_math_calculator", MathTools::get_basic_math_calculator_definition, MathTools::run_basic_math_calculator}
//        {"get_current_time",   TimeTools::get_current_time_definition,   TimeTools::run_get_current_time},
//        {"read_text_lines",    FileTools::get_read_text_lines_definition, FileTools::run_read_text_lines},
//        {"write_text_file",    FileTools::get_write_text_file_definition, FileTools::run_write_text_file},
//        {"validate_utf8_file", FileTools::get_validate_utf8_file_definition, FileTools::run_validate_utf8_file},
//        {"list_directory",     SystemTools::get_list_directory_definition, SystemTools::run_list_directory},
//        {"path_stat",          SystemTools::get_path_stat_definition,      SystemTools::run_path_stat},
//        {"grep",               SystemTools::get_grep_definition,           SystemTools::run_grep},
//        {"glob",               SystemTools::get_glob_definition,           SystemTools::run_glob},
    };
    return registry;
}

} // namespace BuiltinTools
