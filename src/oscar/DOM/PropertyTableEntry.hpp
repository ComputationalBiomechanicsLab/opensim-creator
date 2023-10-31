#pragma once

#include <oscar/DOM/PropertyDescription.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/Variant.hpp>

namespace osc
{
    class PropertyTableEntry final {
    public:
        explicit PropertyTableEntry(PropertyDescription const& desc) :
            m_Name{desc.getName()},
            m_DefaultValue{desc.getDefaultValue()}
        {
        }

        StringName const& getName() const
        {
            return m_Name;
        }

        Variant const& getDefaultValue() const
        {
            return m_DefaultValue;
        }

        Variant const& getValue() const
        {
            return m_Value;
        }

        void setValue(Variant const& newValue)
        {
            if (newValue.getType() == m_Value.getType())
            {
                m_Value = newValue;
            }
        }
    private:
        StringName m_Name;
        Variant m_DefaultValue;
        Variant m_Value = m_DefaultValue;
    };
}
