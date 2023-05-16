#pragma once

#include "src/Utils/PropertySystem/AbstractProperty.hpp"

namespace osc
{
    // PROPERTY
    //
    // - a single instance
    // - of a type selected from a compile-time set of simple types (e.g. float, string)
    // - that is always a direct member of a component class

    // typed base class for a property
    template<typename TValue>
    class Property : public AbstractProperty {
    public:
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