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
        // mostly useful for `impl_custom_setter`, because it allows implementations
        // to (e.g.) coerce property values
        void set_property_value_raw(const StringName& property_name, const Variant& value);
    public:

        // gets the `Class` (osc) of the `Object` type (C++)
        //
        // derived types that inherit from `Object` (C++) should ensure that
        // their associated `Class` (osc) this `Class` as the parent.
        static const Class& klass_static();

        virtual ~Object() noexcept = default;

        std::string to_string() const
        {
            return impl_to_string();
        }

        std::unique_ptr<Object> clone() const
        {
            return impl_clone();
        }

        const Class& klass() const
        {
            return klass_;
        }

        size_t num_properties() const;
        const StringName& property_name(size_t property_index) const;

        std::optional<size_t> property_index(const StringName& property_name) const;
        const Variant* property_default_value(const StringName& property_name) const;
        const Variant& property_default_value_or_throw(const StringName& property_name) const;
        const Variant* property_value(const StringName& property_name) const;
        const Variant& property_value_or_throw(const StringName& property_name) const;
        bool set_property_value(const StringName& property_name, const Variant& value);
        void set_property_value_or_throw(const StringName& property_name, const Variant& value);

    private:
        virtual std::string impl_to_string() const;
        virtual std::unique_ptr<Object> impl_clone() const = 0;

        // override this method to implement custom behavior when a property is set on
        // this object
        //
        // - return `true` if your implementation has "handled" the `set` call (i.e. so
        //   that `Object` knows that it does not need to do anything further)
        //
        // - return `false` if your implementation did not handle the `set` call and, therefore,
        //   `Object` should handle it instead
        virtual bool impl_custom_property_setter(const StringName& property_name, const Variant& value);

        Class klass_;
        std::vector<Variant> property_values_;
    };

    inline std::string to_string(const Object& obj)
    {
        return obj.to_string();
    }
}
