#pragma once

#include "builtin-tools/common/types.h"

#include <vector>

namespace BuiltinTools {

struct ToolRegistryEntry {
    const char* name;
    BuiltinTools::Types::ToolDefinition (*definition)();
    json (*runner)(const json&);
};

const std::vector<ToolRegistryEntry>& getToolRegistry();

} // namespace BuiltinTools
