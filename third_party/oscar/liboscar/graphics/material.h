#pragma once

#include <liboscar/graphics/blending_equation.h>
#include <liboscar/graphics/cull_mode.h>
#include <liboscar/graphics/depth_function.h>
#include <liboscar/graphics/destination_blending_factor.h>
#include <liboscar/graphics/material_property_value.h>
#include <liboscar/graphics/shader.h>
#include <liboscar/graphics/source_blending_factor.h>
#include <liboscar/utils/copy_on_upd_shared_value.h>
#include <liboscar/utils/string_name.h>

#include <concepts>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace osc
{
    class Material {
    public:
        explicit Material(Shader);

        const Shader& shader() const;

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

        bool is_transparent() const;
        void set_transparent(bool);

        SourceBlendingFactor source_blending_factor() const;
        void set_source_blending_factor(SourceBlendingFactor);

        DestinationBlendingFactor destination_blending_factor() const;
        void set_destination_blending_factor(DestinationBlendingFactor);

        BlendingEquation blending_equation() const;
        void set_blending_equation(BlendingEquation);

        bool is_depth_tested() const;
        void set_depth_tested(bool);

        DepthFunction depth_function() const;
        void set_depth_function(DepthFunction);

        bool writes_to_depth_buffer() const;
        void set_writes_to_depth_buffer(bool);

        bool is_wireframe() const;
        void set_wireframe(bool);

        CullMode cull_mode() const;
        void set_cull_mode(CullMode);

        friend bool operator==(const Material&, const Material&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, const Material&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdSharedValue<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Material&);
}
