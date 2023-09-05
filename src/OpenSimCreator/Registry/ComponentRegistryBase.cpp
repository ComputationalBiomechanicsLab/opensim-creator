#include "ComponentRegistryBase.hpp"

#include <OpenSim/Common/Component.h>

#include <cstddef>
#include <optional>
#include <typeinfo>

std::optional<size_t> osc::IndexOf(
    ComponentRegistryBase const& registry,
    OpenSim::Component const& component)
{
    for (size_t i = 0; i < registry.size(); ++i)
    {
        if (typeid(registry[i].prototype()) == typeid(component))
        {
            return i;
        }
    }
    return std::nullopt;
}
