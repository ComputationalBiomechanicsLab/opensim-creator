#pragma once

#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntryBase.h>

#include <OpenSim/Common/Component.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace osc
{
    class ComponentRegistryBase {
    public:
        using value_type = ComponentRegistryEntryBase;

        CStringView name() const { return m_Name; }
        CStringView description() const { return m_Description; }

        value_type const* begin() const { return m_Entries.data(); }
        value_type const* end() const { return m_Entries.data() + m_Entries.size(); }
        size_t size() const { return m_Entries.size(); }
        value_type const& operator[](size_t i) const { return m_Entries[i]; }

    protected:
        ComponentRegistryBase(
            std::string_view name_,
            std::string_view description_) :

            m_Name{name_},
            m_Description{description_}
        {}

        ComponentRegistryEntryBase& push_back_erased(ComponentRegistryEntryBase&& el)
        {
            return m_Entries.emplace_back(std::move(el));
        }

    private:
        std::string m_Name;
        std::string m_Description;
        std::vector<ComponentRegistryEntryBase> m_Entries;
    };

    std::optional<size_t> IndexOf(ComponentRegistryBase const&, OpenSim::Component const&);

    template<typename T>
    std::optional<size_t> IndexOf(ComponentRegistryBase const& registry)
    {
        for (size_t i = 0; i < registry.size(); ++i) {
            OpenSim::Component const& prototype = registry[i].prototype();
            if (typeid(prototype) == typeid(T)) {
                return i;
            }
        }
        return std::nullopt;
    }
}
