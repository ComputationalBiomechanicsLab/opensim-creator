#pragma once

#include <oscar/Platform/AppSettingValueType.hpp>

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

        explicit AppSettingValue(bool value_) :
            m_Value{value_}
        {
        }

        AppSettingValueType type() const;
        bool toBool() const;  // returns `false` if type is non-boolean
        std::string toString() const;
    private:
        friend bool operator==(AppSettingValue const&, AppSettingValue const&) noexcept;
        friend bool operator!=(AppSettingValue const&, AppSettingValue const&) noexcept;
        std::variant<std::string, bool> m_Value;
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
