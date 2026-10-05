#pragma once

#include <libopynsim/component_registry/component_registry_entry_base.h>

#include <liboscar/utilities/c_string_view.h>
#include <OpenSim/Common/Component.h>

#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace opyn
{
    // Represents a type-erased sequence of named/described `OpenSim::Component`s.
    class ComponentRegistryBase {
    public:
        using value_type = ComponentRegistryEntryBase;
        using reference = value_type&;
        using const_reference = const value_type&;
        using const_iterator = const value_type*;
        using size_type = size_t;

        osc::CStringView name() const { return name_; }
        osc::CStringView description() const { return description_; }

        const_iterator begin() const { return entries_.data(); }
        const_iterator end() const { return entries_.data() + entries_.size(); }
        size_type size() const { return entries_.size(); }
        const_reference operator[](size_type pos) const { return entries_[pos]; }

    protected:
        explicit ComponentRegistryBase(std::string_view name, std::string_view description) :
            name_{name},
            description_{description}
        {}

        template<typename... Args>
        requires std::constructible_from<ComponentRegistryEntryBase, Args&&...>
        reference emplace_back_erased(Args&&... args)
        {
            return entries_.emplace_back(std::forward<Args>(args)...);
        }

    private:
        std::string name_;
        std::string description_;
        std::vector<ComponentRegistryEntryBase> entries_;
    };

    std::optional<size_t> index_of(const ComponentRegistryBase&, std::string_view class_name);
    std::optional<size_t> index_of(const ComponentRegistryBase&, const OpenSim::Component&);

    template<typename T>
    std::optional<size_t> index_of(const ComponentRegistryBase& registry)
    {
        for (size_t i = 0; i < registry.size(); ++i) {
            const OpenSim::Component& prototype = registry[i].prototype();
            if (typeid(prototype) == typeid(T)) {
                return i;
            }
        }
        return std::nullopt;
    }
}
