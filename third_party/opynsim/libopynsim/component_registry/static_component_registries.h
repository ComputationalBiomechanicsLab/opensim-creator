#pragma once

namespace OpenSim { class Component; }
namespace opyn { template<typename> class ComponentRegistry; }

namespace opyn
{
    template<typename T>
    const ComponentRegistry<T>& GetComponentRegistry();

    const ComponentRegistry<OpenSim::Component>& GetCustomComponentRegistry();

    const ComponentRegistry<OpenSim::Component>& GetAllRegisteredComponents();
}
