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

        explicit AppSettingValue(bool value_) :
            m_Value{value_}
        {
        }

        AppSettingValueType type() const;
        bool toBool() const;  // returns `false` if type is non-boolean
        std::string toString() const;
    private:
        std::variant<std::string, bool> m_Value;
    };
}
