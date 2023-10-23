#pragma once

#include <oscar_document/PropertyTable.hpp>
#include <oscar_document/VariantType.hpp>

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace osc::doc { class PropertyDescriptions; }
namespace osc::doc { class Variant; }

namespace osc::doc
{
    class Object {
    protected:
        Object() = default;
        explicit Object(PropertyDescriptions const&);
        Object(Object const&) = default;
        Object(Object&&) noexcept = default;
        Object& operator=(Object const&) = default;
        Object& operator=(Object&&) noexcept = default;

        void setPropertyDescriptions(PropertyDescriptions const&);
    public:
        virtual ~Object() noexcept = default;

        std::string toString() const { return implToString(); }
        std::unique_ptr<Object> clone() const { return implClone(); }

        size_t getNumProperties() const;
        CStringView getPropertyName(size_t propertyIndex) const;
        VariantType getPropertyType(size_t propertyIndex) const;
        Variant const& getPropertyValue(size_t propertyIndex) const;
        void setPropertyValue(size_t propertyIndex, Variant const&);
        bool isPropertyValueDefaulted(size_t propertyIndex) const;
        Variant const& getPropertyDefaultValue(size_t propertyIndex) const;

        bool hasProperty(std::string_view propertyName) const;
        std::optional<size_t> getPropertyIndex(std::string_view propertyName) const;
        Variant const* tryGetPropertyValue(std::string_view propertyName) const;
        bool trySetPropertyValue(std::string_view propertyName, Variant const& newValue);
        Variant const& getPropertyValue(std::string_view propertyName);
        void setPropertyValue(std::string_view propertyName, Variant const& newValue);

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
