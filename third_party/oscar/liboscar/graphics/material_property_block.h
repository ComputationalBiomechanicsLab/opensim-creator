#pragma once

#include <liboscar/graphics/material_property_value.h>
#include <liboscar/utils/copy_on_upd_shared_value.h>
#include <liboscar/utils/string_name.h>

#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

namespace osc
{
    // material property block
    //
    // enables callers to apply per-instance properties when using a material (more efficiently
    // than using a different `Material` every time)
    class MaterialPropertyBlock {
    public:
        MaterialPropertyBlock();

        void clear();
        [[nodiscard]] bool empty() const;

        template<MaterialPropertyValue T> std::optional<T> get(std::string_view property_name) const;
        template<MaterialPropertyValue T> std::optional<T> get(const StringName& property_name) const;
        template<MaterialPropertyValue T> void set(std::string_view property_name, const T& value);
        template<MaterialPropertyValue T> void set(const StringName& property_name, const T& value);
        template<MaterialPropertyValue T> std::optional<std::span<const T>> get_array(std::string_view property_name) const;
        template<MaterialPropertyValue T> std::optional<std::span<const T>> get_array(const StringName& property_name) const;
        template<MaterialPropertyValue T> void set_array(std::string_view property_name, std::span<const T> values);
        template<MaterialPropertyValue T> void set_array(const StringName& property_name, std::span<const T> values);

        // calling `set_array` without a type argument (e.g. `set_array(prop, some_range)`) deduces
        // the type from the range in order to call `set_array<T>(prop, some_range)`
        template<
            std::convertible_to<std::string_view> StringLike,
            std::ranges::contiguous_range Range
        >
        void set_array(StringLike&& property_name, Range&& values)
        {
            set_array<std::ranges::range_value_t<Range>>(std::forward<StringLike>(property_name), std::forward<Range>(values));
        }

        void unset(std::string_view property_name);
        void unset(const StringName& property_name);

    private:
        friend bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
        friend std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdSharedValue<Impl> impl_;
    };

    bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
    std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
}
