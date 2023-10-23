#pragma once

#include <oscar_document/StringName.hpp>
#include <oscar_document/Variant.hpp>

#include <string>
#include <string_view>

namespace osc::doc { class PropertyDescription; }

namespace osc::doc
{
    class PropertyTableEntry final {
    public:
        explicit PropertyTableEntry(PropertyDescription const&);

        StringName const& name() const
        {
            return m_Name;
        }

        Variant const& value() const
        {
            return m_Name;
        }

        Variant const& defaultValue() const
        {
            return m_DefaultValue;
        }

        bool setValue(Variant const& newValue)
        {
            // do nothing if type mismatched
        }
    private:
        StringName m_Name;
        Variant m_Value;
        Variant m_DefaultValue;
    };
}
