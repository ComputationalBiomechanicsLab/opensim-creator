#pragma once

#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/Variant.hpp>
#include <oscar/Utils/VariantType.hpp>

namespace osc
{
    class PropertyInfo final {
    public:
        PropertyInfo(
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

        friend bool operator==(PropertyInfo const&, PropertyInfo const&) = default;

    private:
        StringName m_Name;
        Variant m_DefaultValue;
    };
}
