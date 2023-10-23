#pragma once

#include <oscar_document/PropertyTableEntry.hpp>
#include <oscar_document/StringName.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

namespace osc::doc { class PropertyDescription; }
namespace osc::doc { class Variant; }

namespace osc::doc
{
    class PropertyTable final {
    public:
        PropertyTable() = default;
        explicit PropertyTable(nonstd::span<PropertyDescription const>);

        size_t size() const { return m_Entries.size(); }
        PropertyTableEntry const& operator[](size_t propertyIndex) const { return m_Entries[propertyIndex]; }
        std::optional<size_t> indexOf(StringName const& propertyName) const;
        void setValue(size_t propertyIndex, Variant const& newPropertyValue);

    private:
        std::vector<PropertyTableEntry> m_Entries;
        std::unordered_map<StringName, size_t> m_NameToEntryLookup;
    };
}
