#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/CullMode.h>
#include <oscar/Graphics/DepthFunction.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <concepts>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace osc
{
    class Material final {
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

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<Mat3> get_mat3(StringLike&& property_name) const
        {
            return properties().get_mat3(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_mat3(StringLike&& property_name, const Mat3& mat)
        {
            upd_properties().set_mat3(std::forward<StringLike>(property_name), mat);
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<Mat4> get_mat4(StringLike&& property_name) const
        {
            return properties().get_mat4(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_mat4(StringLike&& property_name, const Mat4& mat)
        {
            upd_properties().set_mat4(std::forward<StringLike>(property_name), mat);
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<std::span<const Mat4>> get_mat4_array(StringLike&& property_name) const
        {
            return properties().get_mat4_array(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_mat4_array(StringLike&& property_name, std::span<const Mat4> value)
        {
            upd_properties().set_mat4_array(std::forward<StringLike>(property_name), value);
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<int32_t> get_int(StringLike&& property_name) const
        {
            return properties().get_int(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_int(StringLike&& property_name, int32_t value)
        {
            upd_properties().set_int(std::forward<StringLike>(property_name), value);
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<bool> get_bool(StringLike&& property_name) const
        {
            return properties().get_bool(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_bool(StringLike&& property_name, bool value)
        {
            upd_properties().set_bool(std::forward<StringLike>(property_name), value);
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<Texture2D> get_texture(StringLike&& property_name) const
        {
            return properties().get_texture(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_texture(StringLike&& property_name, const Texture2D& texture)
        {
            upd_properties().set_texture(std::forward<StringLike>(property_name), texture);
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<RenderTexture> get_render_texture(StringLike&& property_name) const
        {
            return properties().get_render_texture(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_render_texture(StringLike&& property_name, RenderTexture value)
        {
            upd_properties().set_render_texture(std::forward<StringLike>(property_name), std::move(value));
        }

        template<std::convertible_to<std::string_view> StringLike>
        std::optional<Cubemap> get_cubemap(StringLike&& property_name) const
        {
            return properties().get_cubemap(std::forward<StringLike>(property_name));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void set_cubemap(StringLike&& property_name, Cubemap value)
        {
            upd_properties().set_cubemap(std::forward<StringLike>(property_name), std::move(value));
        }

        template<std::convertible_to<std::string_view> StringLike>
        void unset(StringLike&& property_name)
        {
            upd_properties().unset(std::forward<StringLike>(property_name));
        }

        bool is_transparent() const;
        void set_transparent(bool);

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
