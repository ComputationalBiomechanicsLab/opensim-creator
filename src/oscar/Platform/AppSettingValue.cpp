#include "AppSettingValue.h"

#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <string>
#include <variant>

using namespace osc;
using namespace std::literals;

AppSettingValueType osc::AppSettingValue::type() const
{
    static_assert(std::variant_size_v<decltype(value_)> == num_options<AppSettingValueType>());

    return std::visit(Overload
    {
        [](const std::string&) { return AppSettingValueType::String; },
        [](bool)               { return AppSettingValueType::Bool; },
        [](const Color&)       { return AppSettingValueType::Color; },
    }, value_);
}

bool osc::AppSettingValue::to_bool() const
{
    return std::visit(Overload{
        [](const std::string& s)
        {
            if (s.empty()) {
                return false;
            }
            else if (is_equal_case_insensitive(s, "false")) {
                return false;
            }
            else if (is_equal_case_insensitive(s, "0")) {
                return false;
            }
            else {
                return true;
            }
        },
        [](bool v)
        {
            return v;
        },
        [](const Color&)
        {
            return false;
        },
    }, value_);
}

std::string osc::AppSettingValue::to_string() const
{
    return std::visit(Overload{
        [](const std::string& v) { return std::string{v}; },
        [](bool v) { return v ? "true"s : "false"s; },
        [](const Color& c) { return to_html_string_rgba(c); },
    }, value_);
}

Color osc::AppSettingValue::to_color() const
{
    return std::visit(Overload{
        [](const std::string& v) { return try_parse_html_color_string(v).value_or(Color::white()); },
        [](bool v) { return v ? Color::white() : Color::black(); },
        [](const Color& c) { return c; },
    }, value_);
}
