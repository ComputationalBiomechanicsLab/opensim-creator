#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Platform/AppSettingValueType.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <string>
#include <utility>
#include <variant>

namespace osc
{
    class AppSettingValue final {
    public:
        explicit AppSettingValue(std::string value_) :
            m_Value{std::move(value_)}
        {
        }

        explicit AppSettingValue(char const* value_) :
            m_Value{std::string{value_}}
        {
        }

        explicit AppSettingValue(CStringView value_) :
            m_Value{std::string{value_}}
        {
        }

        explicit AppSettingValue(bool value_) :
            m_Value{value_}
        {
        }

        explicit AppSettingValue(Color const& value_) :
            m_Value{value_}
        {
        }

        AppSettingValueType type() const;
        bool toBool() const;
        std::string toString() const;
        Color toColor() const;

        friend bool operator==(AppSettingValue const& lhs, AppSettingValue const& rhs) noexcept
        {
            return lhs.m_Value == rhs.m_Value;
        }

        friend bool operator!=(AppSettingValue const& lhs, AppSettingValue const& rhs) noexcept
        {
            return lhs.m_Value != rhs.m_Value;
        }
    private:
        std::variant<std::string, bool, Color> m_Value;
    };
}
