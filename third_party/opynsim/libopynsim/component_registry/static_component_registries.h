#pragma once

namespace OpenSim { class Component; }
namespace opyn { template<typename> class ComponentRegistry; }

namespace opyn
{
    template<typename T>
    const ComponentRegistry<T>& get_component_registry();
    const ComponentRegistry<OpenSim::Component>& get_opynsim_component_registry();
    const ComponentRegistry<OpenSim::Component>& get_all_registered_components();
}
