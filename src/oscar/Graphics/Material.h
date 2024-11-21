#pragma once

#include <oscar/Graphics/BlendingEquation.h>
#include <oscar/Graphics/CullMode.h>
#include <oscar/Graphics/DepthFunction.h>
#include <oscar/Graphics/DestinationBlendingFactor.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/SourceBlendingFactor.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

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

        template<typename T, std::convertible_to<std::string_view> StringLike>
        std::optional<T> get(StringLike&& property_name) const
        {
            return properties().get<T>(std::forward<StringLike>(property_name));
        }

        template<typename T, std::convertible_to<std::string_view> StringLike>
        void set(StringLike&& property_name, const T& value)
        {
            upd_properties().set<T>(std::forward<StringLike>(property_name), value);
        }

        template<typename T, std::convertible_to<std::string_view> StringLike>
        std::optional<std::span<const T>> get_array(StringLike&& property_name) const
        {
            return properties().get_array<T>(std::forward<StringLike>(property_name));
        }

        template<typename T, std::convertible_to<std::string_view> StringLike>
        void set_array(StringLike&& property_name, std::span<const T> values)
        {
            upd_properties().set_array<T>(std::forward<StringLike>(property_name), values);
        }

        template<
            std::convertible_to<std::string_view> StringLike,
            std::ranges::contiguous_range Range
        >
        void set_array(StringLike&& property_name, Range&& values)
        {
            upd_properties().set_array(std::forward<StringLike>(property_name), std::forward<Range>(values));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void unset(StringLike&& property_name)
        {
            upd_properties().unset(std::forward<StringLike>(property_name));
        }

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

        bool is_wireframe() const;
        void set_wireframe(bool);

        CullMode cull_mode() const;
        void set_cull_mode(CullMode);

        friend bool operator==(const Material&, const Material&) = default;

    private:
        const MaterialPropertyBlock& properties() const;
        MaterialPropertyBlock& upd_properties();

        friend std::ostream& operator<<(std::ostream&, const Material&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Material&);
}
