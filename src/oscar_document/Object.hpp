#pragma once

#include <oscar_document/PropertyDescription.hpp>
#include <oscar_document/PropertyTable.hpp>
#include <oscar_document/StringName.hpp>
#include <oscar_document/Variant.hpp>
#include <oscar_document/VariantType.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace osc::doc
{
    class Object {
    protected:
        Object() = default;
        explicit Object(nonstd::span<PropertyDescription const>);
        Object(Object const&) = default;
        Object(Object&&) noexcept = default;
        Object& operator=(Object const&) = default;
        Object& operator=(Object&&) noexcept = default;
    public:
        virtual ~Object() noexcept = default;

        std::string toString() const { return implToString(); }
        std::unique_ptr<Object> clone() const { return implClone(); }

        size_t getNumProperties() const;
        StringName const& getPropertyName(size_t propertyIndex) const;
        VariantType getPropertyType(size_t propertyIndex) const;
        Variant const& getPropertyValue(size_t propertyIndex) const;
        void setPropertyValue(size_t propertyIndex, Variant const& newPropertyValue);
        Variant const& getPropertyDefaultValue(size_t propertyIndex) const;

        std::optional<size_t> getPropertyIndex(StringName const& propertyName) const;
        Variant const* tryGetPropertyValue(StringName const& propertyName) const;
        bool trySetPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);
        Variant const& getPropertyValue(StringName const& propertyName);
        void setPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);

    private:
        virtual std::string implToString() const;
        virtual std::unique_ptr<Object> implClone() const = 0;

        PropertyTable m_PropertyTable;
    };

    inline std::string to_string(Object const& o)
    {
        return o.toString();
    }
}
