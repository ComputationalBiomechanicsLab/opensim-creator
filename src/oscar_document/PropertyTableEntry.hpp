#pragma once

#include <oscar_document/PropertyDescription.hpp>
#include <oscar_document/StringName.hpp>
#include <oscar_document/Variant.hpp>

namespace osc::doc
{
    class PropertyTableEntry final {
    public:
        explicit PropertyTableEntry(PropertyDescription const& desc) :
            m_Name{desc.getName()},
            m_DefaultValue{desc.getDefaultValue()}
        {
        }

        StringName const& name() const
        {
            return m_Name;
        }

        Variant const& defaultValue() const
        {
            return m_DefaultValue;
        }

        Variant const& value() const
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
