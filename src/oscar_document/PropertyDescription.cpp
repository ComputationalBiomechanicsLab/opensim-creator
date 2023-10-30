#include "PropertyDescription.hpp"

#include <oscar_document/StringName.hpp>
#include <oscar_document/Variant.hpp>

#include <oscar/Utils/StringHelpers.hpp>

#include <sstream>
#include <stdexcept>
#include <utility>

osc::doc::PropertyDescription::PropertyDescription(
    StringName const& name_,
    Variant defaultValue_) :
    m_Name{name_},
    m_DefaultValue{std::move(defaultValue_)}
{
    if (!IsValidIdentifier(m_Name))
    {
        std::stringstream ss;
        ss << m_Name << ": is not a valid name for a property (must be an identifier)";
        throw std::runtime_error{std::move(ss).str()};
    }
}
