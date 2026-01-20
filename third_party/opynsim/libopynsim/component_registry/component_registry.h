#pragma once

#include <libopynsim/component_registry/component_registry_base.h>
#include <libopynsim/component_registry/component_registry_entry.h>

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace osc
{
    // Represents a sequence of named/described `OpenSim::Component`s of type `T`.
    template<typename T>
    class ComponentRegistry final : public ComponentRegistryBase {
    public:
        using value_type = ComponentRegistryEntry<T>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using const_iterator = const value_type*;

        explicit ComponentRegistry(
            std::string_view name_,
            std::string_view description_) :

            ComponentRegistryBase{name_, description_}
        {}

        const_iterator begin() const
        {
            const auto& base = static_cast<const ComponentRegistryBase&>(*this);
            return static_cast<const_iterator>(base.begin());
        }

        const_iterator end() const
        {
            const auto& base = static_cast<const ComponentRegistryBase&>(*this);
            return static_cast<const_iterator>(base.end());
        }

        const_reference operator[](size_t pos) const
        {
            const auto& base = static_cast<const ComponentRegistryBase&>(*this);
            return static_cast<const_reference>(base[pos]);
        }

        template<typename... Args>
        requires std::constructible_from<value_type, Args&&...>
        const_reference emplace_back(Args&&... args)
        {
            auto& erased = emplace_back_erased(std::forward<Args>(args)...);
            return static_cast<reference>(erased);
        }
    };

    template<typename T>
    const ComponentRegistryEntry<T>& At(const ComponentRegistry<T>& registry, size_t i)
    {
        if (i >= registry.size()) {
            throw std::out_of_range{"attempted to access an out-of-bounds registry entry"};
        }
        return registry[i];
    }

    template<typename T>
    const ComponentRegistryEntry<T>& Get(ComponentRegistry<T> const& registry, const T& el)
    {
        if (auto i = IndexOf(registry, el)) {
            return registry[*i];
        }
        else {
            throw std::out_of_range{"attempted to get an element from the registry that does not exist"};
        }
    }

    template<typename T>
    const ComponentRegistryEntry<T>& Get(const ComponentRegistry<T>& registry, std::string_view componentClassName)
    {
        if (auto i = IndexOf(registry, componentClassName)) {
            return registry[*i];
        }
        else {
            throw std::out_of_range{"attempted to get an element from a component registry that does not exist"};
        }
    }
}
