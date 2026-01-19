#include "ComponentRegistryBase.h"

#include <OpenSim/Common/Component.h>

#include <cstddef>
#include <optional>
#include <typeinfo>

std::optional<size_t> osc::IndexOf(
    const ComponentRegistryBase& registry,
    std::string_view componentClassName)
{
    for (size_t i = 0; i < registry.size(); ++i) {
        const OpenSim::Component& prototype = registry[i].prototype();
        if (prototype.getConcreteClassName() == componentClassName) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> osc::IndexOf(
    const ComponentRegistryBase& registry,
    const OpenSim::Component& component)
{
    for (size_t i = 0; i < registry.size(); ++i) {
        const OpenSim::Component& prototype = registry[i].prototype();
        if (typeid(prototype) == typeid(component)) {
            return i;
        }
    }
    return std::nullopt;
}
