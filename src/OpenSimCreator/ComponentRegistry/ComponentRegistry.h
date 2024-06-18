#pragma once

#include <OpenSimCreator/ComponentRegistry/ComponentRegistryBase.h>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntry.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace osc
{
    template<typename T>
    class ComponentRegistry final : public ComponentRegistryBase {
    public:
        using value_type = ComponentRegistryEntry<T>;

        ComponentRegistry(
            std::string_view name_,
            std::string_view description_) :

            ComponentRegistryBase{name_, description_}
        {}

        const value_type* begin() const
        {
            const auto& base = static_cast<const ComponentRegistryBase&>(*this);
            return static_cast<const value_type*>(base.begin());
        }

        const value_type* end() const
        {
            const auto& base = static_cast<const ComponentRegistryBase&>(*this);
            return static_cast<const value_type*>(base.end());
        }

        const value_type& operator[](size_t i) const
        {
            const auto& base = static_cast<const ComponentRegistryBase&>(*this);
            return static_cast<const value_type&>(base[i]);
        }

        ComponentRegistryEntry<T>& emplace_back(
            std::string_view name,
            std::string_view description,
            std::shared_ptr<T const> prototype)
        {
            auto& erased = push_back_erased(ComponentRegistryEntry<T>
            {
                name,
                description,
                std::move(prototype),
            });
            return static_cast<ComponentRegistryEntry<T>&>(erased);
        }
    };

    template<typename T>
    ComponentRegistryEntry<T> const& At(ComponentRegistry<T> const& registry, size_t i)
    {
        if (i >= registry.size()) {
            throw std::out_of_range{"attempted to access an out-of-bounds registry entry"};
        }
        return registry[i];
    }

    template<typename T>
    ComponentRegistryEntry<T> const& Get(ComponentRegistry<T> const& registry, T const& el)
    {
        if (auto i = IndexOf(registry, el)) {
            return registry[*i];
        }
        else {
            throw std::out_of_range{"attempted to get an element from the registry that does not exist"};
        }
    }

    template<typename T>
    ComponentRegistryEntry<T> const& Get(ComponentRegistry<T> const& registry, std::string_view componentClassName)
    {
        if (auto i = IndexOf(registry, componentClassName)) {
            return registry[*i];
        }
        else {
            throw std::out_of_range{"attempted to get an element from a component registry that does not exist"};
        }
    }
}
