#include "ComponentRegistryBase.h"

#include <OpenSim/Common/Component.h>

#include <cstddef>
#include <optional>
#include <typeinfo>

std::optional<size_t> osc::IndexOf(
    ComponentRegistryBase const& registry,
    std::string_view componentClassName)
{
    for (size_t i = 0; i < registry.size(); ++i) {
        OpenSim::Component const& prototype = registry[i].prototype();
        if (prototype.getConcreteClassName() == componentClassName) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> osc::IndexOf(
    ComponentRegistryBase const& registry,
    OpenSim::Component const& component)
{
    for (size_t i = 0; i < registry.size(); ++i) {
        OpenSim::Component const& prototype = registry[i].prototype();
        if (typeid(prototype) == typeid(component)) {
            return i;
        }
    }
    return std::nullopt;
}
