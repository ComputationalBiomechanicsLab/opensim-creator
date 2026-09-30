#include "component_registry_base.h"

#include <OpenSim/Common/Component.h>

#include <cstddef>
#include <optional>
#include <typeinfo>

std::optional<size_t> opyn::index_of(
    const ComponentRegistryBase& registry,
    std::string_view class_name)
{
    for (size_t i = 0; i < registry.size(); ++i) {
        const OpenSim::Component& prototype = registry[i].prototype();
        if (prototype.getConcreteClassName() == class_name) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> opyn::index_of(
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
