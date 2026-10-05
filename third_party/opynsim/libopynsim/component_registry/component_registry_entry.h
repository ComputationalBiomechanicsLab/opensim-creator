#pragma once

#include <libopynsim/component_registry/component_registry_entry_base.h>

#include <memory>
#include <string_view>
#include <utility>

namespace opyn
{
    template<typename T>
    class ComponentRegistryEntry final : public ComponentRegistryEntryBase {
    public:
        ComponentRegistryEntry(
            std::string_view name,
            std::string_view description,
            std::shared_ptr<const T> prototype) :

            ComponentRegistryEntryBase{name, description, std::move(prototype)}
        {}

        const T& prototype() const
        {
            const auto& base = static_cast<const ComponentRegistryEntryBase&>(*this);
            return static_cast<const T&>(base.prototype());
        }

        std::unique_ptr<T> instantiate() const
        {
            const auto& base = static_cast<const ComponentRegistryEntryBase&>(*this);
            return std::unique_ptr<T>{static_cast<T*>(base.instantiate().release())};
        }
    };
}
