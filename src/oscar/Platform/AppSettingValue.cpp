#include "AppSettingValue.hpp"

#include <oscar/Platform/AppSettingValueType.hpp>
#include <oscar/Utils/VariantHelpers.hpp>

#include <string>
#include <variant>

osc::AppSettingValueType osc::AppSettingValue::type() const
{
    AppSettingValueType rv = AppSettingValueType::String;
    std::visit(Overload
    {
        [&rv](std::string const&) { rv = AppSettingValueType::String; },
        [&rv](bool) { rv = AppSettingValueType::Bool; },
    }, m_Value);
    return rv;
}

bool osc::AppSettingValue::toBool() const
{
    bool rv = false;
    std::visit(Overload
    {
        [&rv](std::string const&) {},
        [&rv](bool v) { rv = v; },
    }, m_Value);
    return rv;
}

std::string osc::AppSettingValue::toString() const
{
    std::string rv;
    std::visit(Overload
    {
        [&rv](std::string const& v) { rv = v; },
        [&rv](bool v) { rv = v ? "true" : "false"; },
    }, m_Value);
    return rv;
}
