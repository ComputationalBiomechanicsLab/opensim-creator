#pragma once

#include <liboscar/dom/property_info.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace osc { class StringName; }

namespace osc
{
    // Represents the shared aspects of `Object`s that are instances on this class.
    //
    // In oscar's runtime engine foundation systems, C++ classes should have
    // a single linear inheritance chain with `Object` at the root. Each
    // C++ class should also define one of these `Class` objects, usually behind
    // a static `klass` member function getter. This is so that runtime systems
    // can "see" the inheritance chain and handle (e.g.) inherited properties
    // correctly.
    class Class final {
    public:
        // Constructs a `Class` that represents an `Object` (i.e. the "root" `Class`).
        Class();

        // Constructs a `Class` with the given `name` that inherits from `parent_class`
        // and has the given `properties`.
        explicit Class(
            std::string_view name,
            const Class& parent_class = Class{},
            std::span<const PropertyInfo> properties = std::span<const PropertyInfo>{}
        );

        // Returns the concrete classname of the `Class`.
        const StringName& name() const;

        // Returns the parent `Class` of this `Class`. If the `Class` has no parent (i.e.
        // it's the `Class` that represents `Object`) then `std::nullopt` is returned.
        std::optional<Class> parent_class() const;

        // Returns a read-only view of the `PropertyInfo` for all properties of this `Class`.
        std::span<const PropertyInfo> properties() const;

        // Returns the index of the `PropertyInfo` that has the given `property_name`, or
        // `std::nullopt` if no `PropertyInfo` with that name could be found.
        std::optional<size_t> property_index(const StringName& property_name) const;

        // Checks if the all aspects of the left- and right-hand operands are equal.
        friend bool operator==(const Class&, const Class&);

    private:
        class Impl;
        std::shared_ptr<const Impl> impl_;
    };

    bool operator==(const Class&, const Class&);
}
