#pragma once

#include <libopynsim/component_registry/component_registry_base.h>
#include <libopynsim/component_registry/component_registry_entry.h>

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace opyn
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

        const_reference at(size_t i) const
        {
            if (i >= size()) {
                throw std::out_of_range{"attempted to access an out-of-bounds registry entry"};
            }
            return (*this)[i];
        }

        const_reference entry_with_classname(std::string_view class_name) const
        {
            auto i = IndexOf(*this, class_name);
            if (not i) {
                throw std::out_of_range{"attempted to get an element from a component registry that does not exist"};
            }
            return (*this)[*i];
        }

        template<typename... Args>
        requires std::constructible_from<value_type, Args&&...>
        const_reference emplace_back(Args&&... args)
        {
            auto& erased = emplace_back_erased(std::forward<Args>(args)...);
            return static_cast<reference>(erased);
        }
    };
}
