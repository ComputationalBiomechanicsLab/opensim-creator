#pragma once

#include <oscar_document/StringName.hpp>
#include <oscar_document/Variant.hpp>
#include <oscar_document/VariantType.hpp>

#include <string_view>

namespace osc::doc
{
    class PropertyDescription final {
    public:
        PropertyDescription(
            StringName const& name_,
            Variant const& defaultValue_) :
            m_Name{name_},
            m_DefaultValue{defaultValue_}
        {
        }

        StringName const& getName() const
        {
            return m_Name;
        }

        VariantType getType() const
        {
            return m_DefaultValue.getType();
        }

        Variant const& getDefaultValue() const
        {
            return m_DefaultValue;
        }

        friend bool operator==(PropertyDescription const& lhs, PropertyDescription const& rhs)
        {
            return lhs.m_Name == rhs.m_Name && lhs.m_DefaultValue == rhs.m_DefaultValue;
        }
        friend bool operator!=(PropertyDescription const& lhs, PropertyDescription const& rhs)
        {
            return !(lhs == rhs);
        }

    private:
        StringName m_Name;
        Variant m_DefaultValue;
    };
}
