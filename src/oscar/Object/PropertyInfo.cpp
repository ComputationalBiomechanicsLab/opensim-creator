#include "PropertyInfo.h"

#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Variant/Variant.h>

#include <sstream>
#include <stdexcept>
#include <utility>

osc::PropertyInfo::PropertyInfo(
    StringName name_,
    Variant defaultValue_) :
    m_Name{std::move(name_)},
    m_DefaultValue{std::move(defaultValue_)}
{
    if (!IsValidIdentifier(m_Name))
    {
        std::stringstream ss;
        ss << m_Name << ": is not a valid name for a property (must be an identifier)";
        throw std::runtime_error{std::move(ss).str()};
    }
}
