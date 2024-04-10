#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/CullMode.h>
#include <oscar/Graphics/DepthFunction.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

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

        std::optional<Color> get_color(std::string_view property_name) const;
        void set_color(std::string_view property_name, const Color&);   // note: assumes color is sRGB

        std::optional<std::span<const Color>> get_color_array(std::string_view property_name) const;
        void set_color_array(std::string_view property_name, std::span<const Color>);

        std::optional<float> get_float(std::string_view property_name) const;
        void set_float(std::string_view property_name, float);

        std::optional<std::span<const float>> get_float_array(std::string_view property_name) const;
        void set_float_array(std::string_view property_name, std::span<const float>);

        std::optional<Vec2> get_vec2(std::string_view property_name) const;
        void set_vec2(std::string_view property_name, Vec2);

        std::optional<Vec3> get_vec3(std::string_view property_name) const;
        void set_vec3(std::string_view property_name, Vec3);

        std::optional<std::span<const Vec3>> get_vec3_array(std::string_view property_name) const;
        void set_vec3_array(std::string_view property_name, std::span<const Vec3>);

        std::optional<Vec4> get_vec4(std::string_view property_name) const;
        void set_vec4(std::string_view property_name, Vec4);

        std::optional<Mat3> get_mat3(std::string_view property_name) const;
        void set_mat3(std::string_view property_name, const Mat3&);

        std::optional<Mat4> get_mat4(std::string_view property_name) const;
        void set_mat4(std::string_view property_name, const Mat4&);

        std::optional<std::span<const Mat4>> get_mat4_array(std::string_view property_name) const;
        void set_mat4_array(std::string_view property_name, std::span<const Mat4>);

        std::optional<int32_t> get_int(std::string_view property_name) const;
        void set_int(std::string_view property_name, int32_t);

        std::optional<bool> get_bool(std::string_view property_name) const;
        void set_bool(std::string_view property_name, bool);

        std::optional<Texture2D> get_texture(std::string_view property_name) const;
        void set_texture(std::string_view property_name, Texture2D);
        void clear_texture(std::string_view property_name);

        std::optional<RenderTexture> get_render_texture(std::string_view property_name) const;
        void set_render_texture(std::string_view property_name, RenderTexture);
        void clear_render_texture(std::string_view property_name);

        std::optional<Cubemap> get_cubemap(std::string_view property_name) const;
        void set_cubemap(std::string_view property_name, Cubemap);
        void clear_cubemap(std::string_view property_name);

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

        friend void swap(Material& a, Material& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(const Material&, const Material&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, const Material&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, const Material&);
}
