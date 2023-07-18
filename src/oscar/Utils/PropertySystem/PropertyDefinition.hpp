#pragma once

#include "oscar/Utils/PropertySystem/ComponentMemberOffset.hpp"
#include "oscar/Utils/PropertySystem/Property.hpp"
#include "oscar/Utils/PropertySystem/PropertyMetadata.hpp"
#include "oscar/Utils/PropertySystem/PropertyType.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <limits>
#include <utility>

namespace osc
{
    class AbstractProperty;
    class Component;
    void RegisterPropertyInParent(
        Component& parent,
        AbstractProperty const&,
        ComponentMemberOffset
    );

    // concrete class that defines a property member in a component
    //
    // (used by `OSC_PROPERTY`)
    template<
        typename TParent,
        typename TValue,
        auto FuncGetOffsetInParent,
        char const* VName,
        char const* VDescription
    >
    class PropertyDefinition final : public Property<TValue> {
    public:
        explicit PropertyDefinition(TValue initialValue_) :
            m_Value{std::move(initialValue_)}
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            RegisterPropertyInParent(
                implUpdOwner(),
                *this,
                static_cast<ComponentMemberOffset>(FuncGetOffsetInParent())
            );
        }

    private:
        Component const& implGetOwner() const final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char const* thisMemLocation = reinterpret_cast<char const*>(this);
            return *reinterpret_cast<Component const*>(thisMemLocation - FuncGetOffsetInParent());
        }

        Component& implUpdOwner() final
        {
            static_assert(FuncGetOffsetInParent() < std::numeric_limits<ComponentMemberOffset>::max(), "might not fit into component's offset table");
            char* thisMemLocation = reinterpret_cast<char*>(this);
            return *reinterpret_cast<Component*>(thisMemLocation - FuncGetOffsetInParent());
        }

        CStringView implGetName() const final
        {
            return VName;
        }

        CStringView implGetDescription() const final
        {
            return VDescription;
        }

        PropertyType implGetPropertyType() const final
        {
            return PropertyMetadata<TValue>::type();
        }

        TValue const& implGetValue() const final
        {
            return m_Value;
        }

        TValue& implUpdValue() final
        {
            return m_Value;
        }

        TValue m_Value;
    };
}
