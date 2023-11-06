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

        // implementors, override this method to customize the behavior of `get` functions
        //
        // - return non-`nullptr` for callers to get a pointer/reference to the value you returned
        // - return `nullptr` if you want `Object` to try and handle it "normally"
        virtual Variant const* implCustomPropertyGetter(StringName const& propertyName) const;

        // implementors, override this method to customize the behavior of `set` functions
        //
        // - you should set `propertyName` to `newPropertyValue` (by whatever your internal datastructures require)
        // - return `true` if you want `Object` to handle the set request "normally" (i.e. search+save it into the property table)
        // - return `false` if want `Object` to do nothing further (e.g. because you've detected an error)
        //
        // this is handy for implementing custom validation or data storage
        virtual bool implCustomPropertySetter(StringName const& propertyName, Variant const& newPropertyValue);

        PropertyTable m_PropertyTable;
    };

    inline std::string to_string(Object const& o)
    {
        return o.toString();
    }
}
