#pragma once

#include <oscar/DOM/PropertyTableEntry.hpp>
#include <oscar/Utils/StringName.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace osc { class PropertyDescription; }
namespace osc { class Variant; }

namespace osc
{
    class PropertyTable final {
    public:
        PropertyTable() = default;
        explicit PropertyTable(std::span<PropertyDescription const>);

        size_t size() const { return m_Entries.size(); }
        PropertyTableEntry const& operator[](size_t propertyIndex) const { return m_Entries[propertyIndex]; }
        std::optional<size_t> indexOf(StringName const& propertyName) const;
        void setValue(size_t propertyIndex, Variant const& newPropertyValue);

    private:
        std::vector<PropertyTableEntry> m_Entries;
        std::unordered_map<StringName, size_t> m_NameToEntryLookup;
    };
}
