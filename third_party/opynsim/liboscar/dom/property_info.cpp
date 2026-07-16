#include "property_info.h"

#include <liboscar/utilities/string_helpers.h>
#include <liboscar/utilities/string_name.h>
#include <liboscar/variant/variant.h>

#include <format>
#include <stdexcept>
#include <utility>

osc::PropertyInfo::PropertyInfo(
    StringName name,
    Variant default_value) :

    name_{std::move(name)},
    default_value_{std::move(default_value)}
{
    if (not is_valid_identifier(name_)) {
        throw std::runtime_error{std::format("{}: is not a valid name for a property (must be an identifier)", name_.name())};
    }
}
