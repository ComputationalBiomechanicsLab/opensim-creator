#pragma once

#include <oscar/DOM/PropertyTable.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/VariantType.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace osc { class PropertyDescription; }
namespace osc { class Variant; }

namespace osc
{
    class Object {
    protected:
        Object() = default;
        explicit Object(std::span<PropertyDescription const>);
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

        std::optional<size_t> getPropertyIndex(StringName const& propertyName) const;
        Variant const* tryGetPropertyDefaultValue(StringName const& propertyName) const;
        Variant const& getPropertyDefaultValue(StringName const&) const;
        Variant const* tryGetPropertyValue(StringName const& propertyName) const;
        Variant const& getPropertyValue(StringName const& propertyName) const;
        bool trySetPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);
        void setPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);

    private:
        virtual std::string implToString() const;
        virtual std::unique_ptr<Object> implClone() const = 0;
        virtual Variant const* implCustomPropertyGetter(StringName const& propertyName) const;
        virtual bool implCustomPropertySetter(StringName const& propertyName, Variant const& newPropertyValue);

        PropertyTable m_PropertyTable;
    };

    inline std::string to_string(Object const& o)
    {
        return o.toString();
    }
}
