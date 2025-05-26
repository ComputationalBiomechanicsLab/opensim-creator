#pragma once

#include <libopensimcreator/ComponentRegistry/ComponentRegistryEntryBase.h>

#include <liboscar/Utils/CStringView.h>
#include <OpenSim/Common/Component.h>

#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace osc
{
    // Represents a type-erased sequence of named/described `OpenSim::Component`s.
    class ComponentRegistryBase {
    public:
        using value_type = ComponentRegistryEntryBase;
        using reference = value_type&;
        using const_reference = const value_type&;
        using const_iterator = const value_type*;
        using size_type = size_t;

        CStringView name() const { return m_Name; }
        CStringView description() const { return m_Description; }

        const_iterator begin() const { return m_Entries.data(); }
        const_iterator end() const { return m_Entries.data() + m_Entries.size(); }
        size_type size() const { return m_Entries.size(); }
        const_reference operator[](size_type pos) const { return m_Entries[pos]; }

    protected:
        explicit ComponentRegistryBase(
            std::string_view name_,
            std::string_view description_) :

            m_Name{name_},
            m_Description{description_}
        {}

        template<typename... Args>
        requires std::constructible_from<ComponentRegistryEntryBase, Args&&...>
        reference emplace_back_erased(Args&&... args)
        {
            return m_Entries.emplace_back(std::forward<Args>(args)...);
        }

    private:
        std::string m_Name;
        std::string m_Description;
        std::vector<ComponentRegistryEntryBase> m_Entries;
    };

    std::optional<size_t> IndexOf(const ComponentRegistryBase&, std::string_view componentClassName);
    std::optional<size_t> IndexOf(const ComponentRegistryBase&, const OpenSim::Component&);

    template<typename T>
    std::optional<size_t> IndexOf(const ComponentRegistryBase& registry)
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

