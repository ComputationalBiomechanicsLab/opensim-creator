#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Platform/AppSettingValueType.h>
#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>
#include <variant>

namespace osc
{
    class AppSettingValue final {
    public:
        explicit AppSettingValue(std::string value) :
            value_{value}
        {}

        explicit AppSettingValue(const char* value) :
            value_{std::string{value}}
        {}

        explicit AppSettingValue(CStringView value) :
            value_{std::string{value}}
        {}

        explicit AppSettingValue(bool value) :
            value_{value}
        {}

        explicit AppSettingValue(const Color& value) :
            value_{value}
        {}

        AppSettingValueType type() const;
        bool to_bool() const;
        std::string to_string() const;
        Color to_color() const;

        friend bool operator==(const AppSettingValue&, const AppSettingValue&) = default;

    private:
        std::variant<std::string, bool, Color> value_;
    };
}
