#pragma once

#include "src/Utils/PropertySystem/AbstractProperty.hpp"

namespace osc { template<typename TParent, typename TValue, auto FuncGetOffsetInParent, char const* VName, char const* VDescription> class PropertyDefinition; }

namespace osc
{
    // PROPERTY
    //
    // - a single instance
    // - of a type selected from a compile-time set of simple types (e.g. float, string)
    // - that is always a direct member of a component class
    template<typename TValue>
    class Property : public AbstractProperty {
    private:
        // Property<T> can only be constructed via a PropertyDefinition
        template<
            typename TParent,
            typename TValue,
            auto FuncGetOffsetInParent,
            char const* VName,
            char const* VDescription
        >
        friend class PropertyDefinition;

        Property() = default;
        Property(Property const&) = default;
        Property(Property&&) noexcept = default;
        Property& operator=(Property const&) = default;
        Property& operator=(Property&&) noexcept = default;
    public:
        virtual ~Property() noexcept override = default;

        TValue const& getValue() const
        {
            return implGetValue();
        }

        TValue& updValue()
        {
            return implUpdValue();
        }

        TValue const* operator->() const
        {
            return &implGetValue();
        }

        TValue* operator->()
        {
            return &implUpdValue();
        }

        TValue const& operator*() const
        {
            return implGetValue();
        }

        TValue& operator*()
        {
            return implUpdValue();
        }

    private:
        virtual TValue const& implGetValue() const = 0;
        virtual TValue& implUpdValue() = 0;
    };
}