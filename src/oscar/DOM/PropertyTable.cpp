#include "PropertyTable.hpp"

#include <oscar/DOM/PropertyDescription.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/Variant.hpp>

#include <cstddef>
#include <optional>
#include <span>

osc::PropertyTable::PropertyTable(std::span<PropertyDescription const> descriptions)
{
    if (descriptions.empty())
    {
        return;
    }

    m_Entries.reserve(descriptions.size());
    m_NameToEntryLookup.reserve(descriptions.size());

    // insert backwards (later entries 'override' earlier ones)
    for (auto it = descriptions.rbegin(); it != descriptions.rend(); ++it)
    {
        if (m_NameToEntryLookup.try_emplace(it->getName(), m_Entries.size()).second)
        {
            m_Entries.emplace_back(*it);
        }
    }

    // reverse, so that the properties are in the right order
    std::reverse(m_Entries.begin(), m_Entries.end());

    // reverse indices
    for (auto& [k, v] : m_NameToEntryLookup)
    {
        v = (m_Entries.size()-1) - v;
    }
}

std::optional<size_t> osc::PropertyTable::indexOf(StringName const& propertyName) const
{
    if (auto const it = m_NameToEntryLookup.find(propertyName); it != m_NameToEntryLookup.end())
    {
        return it->second;
    }
    else
    {
        return std::nullopt;
    }
}

void osc::PropertyTable::setValue(size_t propertyIndex, Variant const& newPropertyValue)
{
    m_Entries[propertyIndex].setValue(newPropertyValue);
}
