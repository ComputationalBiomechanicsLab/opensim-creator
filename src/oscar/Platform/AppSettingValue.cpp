#include "AppSettingValue.h"

#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <string>
#include <variant>

using namespace osc;

AppSettingValueType osc::AppSettingValue::type() const
{
    static_assert(std::variant_size_v<decltype(m_Value)> == NumOptions<AppSettingValueType>());

    AppSettingValueType rv = AppSettingValueType::String;
    std::visit(Overload
    {
        [&rv](std::string const&) { rv = AppSettingValueType::String; },
        [&rv](bool) { rv = AppSettingValueType::Bool; },
        [&rv](Color const&) { rv = AppSettingValueType::Color; },
    }, m_Value);
    return rv;
}

bool osc::AppSettingValue::toBool() const
{
    bool rv = false;
    std::visit(Overload
    {
        [&rv](std::string const& s)
        {
            if (s.empty())
            {
                rv = false;
            }
            else if (IsEqualCaseInsensitive(s, "false"))
            {
                rv = false;
            }
            else if (IsEqualCaseInsensitive(s, "0"))
            {
                rv = false;
            }
            else
            {
                rv = true;
            }
        },
        [&rv](bool v) { rv = v; },
        [](Color const&) {},
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
        [&rv](Color const& c) { rv = toHtmlStringRGBA(c); },
    }, m_Value);
    return rv;
}

Color osc::AppSettingValue::toColor() const
{
    Color rv = Color::white();
    std::visit(Overload
    {
        [&rv](std::string const& v)
        {
            if (auto c = tryParseHtmlString(v))
            {
                rv = *c;
            }
        },
        [](bool) {},
        [&rv](Color const& c) { rv = c; },
    }, m_Value);
    return rv;
}
