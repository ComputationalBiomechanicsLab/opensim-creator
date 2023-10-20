#pragma once

#include <oscar_document/VariantType.hpp>

#include <string>
#include <string_view>

namespace osc::doc
{
    class PropertyDescription final {
    public:
        PropertyDescription(
            std::string_view name_,
            VariantType type_) :
            m_Name{name_},
            m_Type{type_}
        {
        }

        std::string_view getName() const
        {
            return m_Name;
        }

        VariantType getType() const
        {
            return m_Type;
        }

        friend bool operator==(PropertyDescription const& lhs, PropertyDescription const& rhs)
        {
            return lhs.m_Name == rhs.m_Name && lhs.m_Type == rhs.m_Type;
        }
        friend bool operator!=(PropertyDescription const& lhs, PropertyDescription const& rhs)
        {
            return !(lhs == rhs);
        }
    private:
        std::string m_Name;
        VariantType m_Type;
    };
}
