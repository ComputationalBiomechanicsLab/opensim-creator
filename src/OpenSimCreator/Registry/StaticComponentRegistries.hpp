#pragma once

namespace osc { template<typename> class ComponentRegistry; }

namespace osc
{
    template<typename T>
    ComponentRegistry<T> const& GetComponentRegistry();
}
