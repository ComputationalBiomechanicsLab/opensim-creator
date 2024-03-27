#pragma once

#include <oscar/DOM/Class.h>
#include <oscar/Variant/Variant.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace osc { class StringName; }

namespace osc
{
    class Object {
    protected:
        explicit Object(const Class&);
        Object(const Object&) = default;
        Object(Object&&) noexcept = default;
        Object& operator=(const Object&) = default;
        Object& operator=(Object&&) noexcept = default;

        // directly sets a property's value in this object
        //
        // mostly useful for `implCustomSetter`, because it allows implementations
        // to (e.g.) coerce property values
        void setPropertyValueRaw(const StringName& propertyName, const Variant& newPropertyValue);
    public:

        // gets the `Class` (osc) of the `Object` type (C++)
        //
        // derived types that inherit from `Object` (C++) should ensure that
        // their associated `Class` (osc) this `Class` as the parent.
        static const Class& getClassStatic();

        virtual ~Object() noexcept = default;

        std::string toString() const
        {
            return implToString();
        }

        std::unique_ptr<Object> clone() const
        {
            return implClone();
        }

        const Class& getClass() const
        {
            return m_Class;
        }

        size_t getNumProperties() const;
        const StringName& getPropertyName(size_t propertyIndex) const;

        std::optional<size_t> getPropertyIndex(const StringName& propertyName) const;
        const Variant* tryGetPropertyDefaultValue(const StringName& propertyName) const;
        const Variant& getPropertyDefaultValue(const StringName& propertyName) const;
        const Variant* tryGetPropertyValue(const StringName& propertyName) const;
        const Variant& getPropertyValue(const StringName& propertyName) const;
        bool trySetPropertyValue(const StringName& propertyName, const Variant& newPropertyValue);
        void setPropertyValue(const StringName& propertyName, const Variant& newPropertyValue);

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
        virtual bool implCustomSetter(const StringName& propertyName, const Variant& newPropertyValue);

        Class m_Class;
        std::vector<Variant> m_PropertyValues;
    };

    inline std::string to_string(const Object& o)
    {
        return o.toString();
    }
}
