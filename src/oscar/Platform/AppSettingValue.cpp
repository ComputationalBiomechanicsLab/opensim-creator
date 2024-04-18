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
    static_assert(std::variant_size_v<decltype(value_)> == num_options<AppSettingValueType>());

    AppSettingValueType rv = AppSettingValueType::String;
    std::visit(Overload
    {
        [&rv](const std::string&) { rv = AppSettingValueType::String; },
        [&rv](bool)               { rv = AppSettingValueType::Bool; },
        [&rv](const Color&)       { rv = AppSettingValueType::Color; },
    }, value_);
    return rv;
}

bool osc::AppSettingValue::to_bool() const
{
    bool rv = false;
    std::visit(Overload{
        [&rv](const std::string& s)
        {
            if (s.empty()) {
                rv = false;
            }
            else if (is_equal_case_insensitive(s, "false")) {
                rv = false;
            }
            else if (is_equal_case_insensitive(s, "0")) {
                rv = false;
            }
            else {
                rv = true;
            }
        },
        [&rv](bool v) { rv = v; },
        [](const Color&) {},
    }, value_);
    return rv;
}

std::string osc::AppSettingValue::to_string() const
{
    std::string rv;
    std::visit(Overload{
        [&rv](const std::string& v) { rv = v; },
        [&rv](bool v) { rv = v ? "true" : "false"; },
        [&rv](const Color& c) { rv = to_html_string_rgba(c); },
    }, value_);
    return rv;
}

Color osc::AppSettingValue::to_color() const
{
    Color rv = Color::white();
    std::visit(Overload{
        [&rv](const std::string& v)
        {
            if (auto c = try_parse_html_color_string(v)) {
                rv = *c;
            }
        },
        [](bool) {},
        [&rv](const Color& c) { rv = c; },
    }, value_);
    return rv;
}
