#pragma once

#include <liboscar/Utils/UID.h>

#include <stdexcept>

namespace OpenSim { class Component; }

namespace osc
{
    class IComponentAccessor {
    protected:
        IComponentAccessor() = default;
        IComponentAccessor(const IComponentAccessor&) = default;
        IComponentAccessor(IComponentAccessor&&) noexcept = default;
        IComponentAccessor& operator=(const IComponentAccessor&) = default;
        IComponentAccessor& operator=(IComponentAccessor&&) noexcept = default;

        friend bool operator==(const IComponentAccessor&, const IComponentAccessor&) = default;
    public:
        virtual ~IComponentAccessor() noexcept = default;

        const OpenSim::Component& getComponent() const { return implGetComponent(); }
        operator const OpenSim::Component& () const { return getComponent(); }

        bool isReadonly() const { return not implCanUpdComponent(); }
        bool canUpdComponent() const { return implCanUpdComponent(); }
        OpenSim::Component& updComponent() { return implUpdComponent(); }

    private:
        // Implementors should return a const reference to an initialized (finalized properties, etc.) component
        virtual const OpenSim::Component& implGetComponent() const = 0;

        // Implementors may return whether the component contained by the concrete `IComponentAccessor` implementation
        // can be modified in-place.
        //
        // If the response can be `true`, implementors must also override `implUpdComponent` accordingly.
        virtual bool implCanUpdComponent() const { return false; }

        // Implementors may return a mutable reference to the contained component. It is up to the caller
        // of `updComponent` to ensure that the component is still valid + initialized after modification.
        //
        // If this is implemented, implementors should override `implCanUpdComponent` accordingly.
        virtual OpenSim::Component& implUpdComponent()
        {
            throw std::runtime_error{"component updating not implemented for this IComponentAccessor"};
        }
    };
}
