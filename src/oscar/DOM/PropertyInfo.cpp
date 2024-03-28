#include "PropertyInfo.h"

#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Variant/Variant.h>

#include <sstream>
#include <stdexcept>
#include <utility>

osc::PropertyInfo::PropertyInfo(
    StringName name,
    Variant default_value) :

    name_{std::move(name)},
    default_value_{std::move(default_value)}
{
    if (not is_valid_identifier(name_)) {
        std::stringstream ss;
        ss << name_ << ": is not a valid name for a property (must be an identifier)";
        throw std::runtime_error{std::move(ss).str()};
    }
}
