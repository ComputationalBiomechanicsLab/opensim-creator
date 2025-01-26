#pragma once

#include <liboscar/Variant/Variant.h>

#include <iosfwd>
#include <string>
#include <unordered_map>

namespace osc
{
    std::unordered_map<std::string, Variant> load_toml_as_flattened_unordered_map(std::istream&);
}
