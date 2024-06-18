#pragma once

#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntryBase.h>

#include <memory>
#include <string_view>
#include <utility>

namespace osc
{
    template<typename T>
    class ComponentRegistryEntry final : public ComponentRegistryEntryBase {
    public:
        ComponentRegistryEntry(
            std::string_view name_,
            std::string_view description_,
            std::shared_ptr<const T> prototype_) :

            ComponentRegistryEntryBase{name_, description_, std::move(prototype_)}
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
