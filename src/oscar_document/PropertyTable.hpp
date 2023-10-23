#pragma once

namespace osc::doc
{
    class PropertyTable final {
    public:
        PropertyTable() = default;
        explicit PropertyTable(PropertyDescriptions const&);

        void setDescriptions(PropertyDescriptions const&);

        size_t size() const;
        PropertyTableEntry const& operator[](size_t) const;
        std::optional<size_t> indexOf(std::string_view propertyName) const;
        bool setValue(size_t, Variant const&);
        bool setValue(std::string_view propertyName, Variant const&);

    private:
        std::vector<PropertyTableEntry> m_Entries;
        std::unordered_map<std::string_view, size_t> m_NameToEntryLookup;
    };
}
