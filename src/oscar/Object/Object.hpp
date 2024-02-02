#pragma once

#include <oscar/Object/Class.hpp>
#include <oscar/Variant/Variant.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace osc { class StringName; }

namespace osc
{
    class Object {
    protected:
        explicit Object(Class const&);
        Object(Object const&) = default;
        Object(Object&&) noexcept = default;
        Object& operator=(Object const&) = default;
        Object& operator=(Object&&) noexcept = default;

        // directly sets a property's value in this object
        //
        // mostly useful for `implCustomSetter`, because it allows implementations
        // to (e.g.) coerce property values
        void setPropertyValueRaw(StringName const& propertyName, Variant const& newPropertyValue);
    public:

        // gets the `Class` (osc) of the `Object` type (C++)
        //
        // derived types that inherit from `Object` (C++) should ensure that
        // their associated `Class` (osc) this `Class` as the parent.
        static Class const& getClassStatic();

        virtual ~Object() noexcept = default;

        std::string toString() const
        {
            return implToString();
        }

        std::unique_ptr<Object> clone() const
        {
            return implClone();
        }

        Class const& getClass() const
        {
            return m_Class;
        }

        size_t getNumProperties() const;
        StringName const& getPropertyName(size_t propertyIndex) const;

        std::optional<size_t> getPropertyIndex(StringName const& propertyName) const;
        Variant const* tryGetPropertyDefaultValue(StringName const& propertyName) const;
        Variant const& getPropertyDefaultValue(StringName const& propertyName) const;
        Variant const* tryGetPropertyValue(StringName const& propertyName) const;
        Variant const& getPropertyValue(StringName const& propertyName) const;
        bool trySetPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);
        void setPropertyValue(StringName const& propertyName, Variant const& newPropertyValue);

    private:
        virtual std::string implToString() const;
        virtual std::unique_ptr<Object> implClone() const = 0;

        // override this method to implement custom behavior when a property is set on
        // this object
        //
        // - return `true` if your implementation has "handled" the `set` call (i.e. so
        //   that `Object` knows that it does not need to do anything further)
        //
        // - return `false` if your implementation did not handle the `set` call and, therefore,
        //   `Object` should handle it instead
        virtual bool implCustomSetter(StringName const& propertyName, Variant const& newPropertyValue);

        Class m_Class;
        std::vector<Variant> m_PropertyValues;
    };

    inline std::string to_string(Object const& o)
    {
        return o.toString();
    }
}
