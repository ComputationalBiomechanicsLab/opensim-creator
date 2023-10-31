#pragma once

#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/Variant.hpp>
#include <oscar/Utils/VariantType.hpp>

namespace osc
{
    class PropertyDescription final {
    public:
        PropertyDescription(
            StringName const& name_,
            Variant defaultValue_
        );

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
