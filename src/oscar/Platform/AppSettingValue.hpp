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
    private:
        friend bool operator==(AppSettingValue const&, AppSettingValue const&) noexcept;
        friend bool operator!=(AppSettingValue const&, AppSettingValue const&) noexcept;
        std::variant<std::string, bool, Color> m_Value;
    };

    inline bool operator==(AppSettingValue const& a, AppSettingValue const& b) noexcept
    {
        return a.m_Value == b.m_Value;
    }

    inline bool operator!=(AppSettingValue const& a, AppSettingValue const& b) noexcept
    {
        return a.m_Value != b.m_Value;
    }
}
