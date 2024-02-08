#pragma once

namespace OpenSim { class Component; }
namespace osc { template<typename> class ComponentRegistry; }

namespace osc
{
    template<typename T>
    ComponentRegistry<T> const& GetComponentRegistry();

    ComponentRegistry<OpenSim::Component> const& GetCustomComponentRegistry();

    ComponentRegistry<OpenSim::Component> const& GetAllRegisteredComponents();
}
