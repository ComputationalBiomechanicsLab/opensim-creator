#pragma once

namespace OpenSim { class Component; }
namespace osc { template<typename> class ComponentRegistry; }

namespace osc
{
    template<typename T>
    const ComponentRegistry<T>& GetComponentRegistry();

    const ComponentRegistry<OpenSim::Component>& GetCustomComponentRegistry();

    const ComponentRegistry<OpenSim::Component>& GetAllRegisteredComponents();
}
