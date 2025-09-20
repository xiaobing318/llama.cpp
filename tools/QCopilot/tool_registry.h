#pragma once

#include "builtin_tools/common/tool_types.h"

#include <vector>

namespace BuiltinTools {

struct ToolRegistryEntry {
    const char* name;
    builtin_tools::ToolDefinition (*definition)();
    json (*runner)(const json&);
};

const std::vector<ToolRegistryEntry>& getToolRegistry();

} // namespace BuiltinTools
