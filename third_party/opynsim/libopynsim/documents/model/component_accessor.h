#pragma once

#include <stdexcept>

namespace OpenSim { class Component; }

namespace opyn
{
    class ComponentAccessor {
    protected:
        ComponentAccessor() = default;
        ComponentAccessor(const ComponentAccessor&) = default;
        ComponentAccessor(ComponentAccessor&&) noexcept = default;
        ComponentAccessor& operator=(const ComponentAccessor&) = default;
        ComponentAccessor& operator=(ComponentAccessor&&) noexcept = default;

        friend bool operator==(const ComponentAccessor&, const ComponentAccessor&) = default;
    public:
        virtual ~ComponentAccessor() noexcept = default;

        const OpenSim::Component& get_component() const { return impl_get_component(); }
        operator const OpenSim::Component& () const { return get_component(); }

        bool is_readonly() const { return not impl_can_upd_component(); }
        bool can_upd_component() const { return impl_can_upd_component(); }
        OpenSim::Component& upd_component() { return impl_upd_component(); }

    private:
        // Implementors should return a const reference to an initialized (finalized properties, etc.) component
        virtual const OpenSim::Component& impl_get_component() const = 0;

        // Implementors may return whether the component contained by the concrete `ComponentAccessor` implementation
        // can be modified in-place.
        //
        // If the response can be `true`, implementors must also override `impl_upd_component` accordingly.
        virtual bool impl_can_upd_component() const { return false; }

        // Implementors may return a mutable reference to the contained component. It is up to the caller
        // of `upd_component` to ensure that the component is still valid + initialized after modification.
        //
        // If this is implemented, implementors should override `impl_can_upd_component` accordingly.
        virtual OpenSim::Component& impl_upd_component()
        {
            throw std::runtime_error{"component updating not implemented for this ComponentAccessor"};
        }
    };
}
