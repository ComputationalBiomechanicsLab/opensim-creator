#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/StringName.h>

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace osc
{
    // material property block
    //
    // enables callers to apply per-instance properties when using a material (more efficiently
    // than using a different `Material` every time)
    class MaterialPropertyBlock final {
    public:
        MaterialPropertyBlock();

        void clear();
        [[nodiscard]] bool empty() const;

        std::optional<Color> get_color(std::string_view property_name) const;
        std::optional<Color> get_color(const StringName& property_name) const;
        void set_color(std::string_view property_name, Color);
        void set_color(const StringName& property_name, Color);

        std::optional<std::span<const Color>> get_color_array(std::string_view property_name) const;
        std::optional<std::span<const Color>> get_color_array(const StringName& property_name) const;
        void set_color_array(std::string_view property_name, std::span<const Color>);
        void set_color_array(const StringName& property_name, std::span<const Color>);

        std::optional<float> get_float(std::string_view property_name) const;
        std::optional<float> get_float(const StringName& property_name) const;
        void set_float(std::string_view property_name, float);
        void set_float(const StringName& property_name, float);

        std::optional<std::span<const float>> get_float_array(std::string_view property_name) const;
        std::optional<std::span<const float>> get_float_array(const StringName& property_name) const;
        void set_float_array(std::string_view property_name, std::span<const float>);
        void set_float_array(const StringName& property_name, std::span<const float>);

        std::optional<Vec2> get_vec2(std::string_view property_name) const;
        std::optional<Vec2> get_vec2(const StringName& property_name) const;
        void set_vec2(std::string_view property_name, Vec2);
        void set_vec2(const StringName& property_name, Vec2);

        std::optional<Vec3> get_vec3(std::string_view property_name) const;
        std::optional<Vec3> get_vec3(const StringName& property_name) const;
        void set_vec3(std::string_view property_name, Vec3);
        void set_vec3(const StringName& property_name, Vec3);

        std::optional<std::span<const Vec3>> get_vec3_array(std::string_view property_name) const;
        std::optional<std::span<const Vec3>> get_vec3_array(const StringName& property_name) const;
        void set_vec3_array(std::string_view property_name, std::span<const Vec3>);
        void set_vec3_array(const StringName& property_name, std::span<const Vec3>);

        std::optional<Vec4> get_vec4(std::string_view property_name) const;
        std::optional<Vec4> get_vec4(const StringName& property_name) const;
        void set_vec4(std::string_view property_name, Vec4);
        void set_vec4(const StringName& property_name, Vec4);

        std::optional<Mat3> get_mat3(std::string_view property_name) const;
        std::optional<Mat3> get_mat3(const StringName& property_name) const;
        void set_mat3(std::string_view property_name, const Mat3&);
        void set_mat3(const StringName& property_name, const Mat3&);

        std::optional<Mat4> get_mat4(std::string_view property_name) const;
        std::optional<Mat4> get_mat4(const StringName& property_name) const;
        void set_mat4(std::string_view, const Mat4&);
        void set_mat4(const StringName& property_name, const Mat4&);

        std::optional<std::span<const Mat4>> get_mat4_array(std::string_view property_name) const;
        std::optional<std::span<const Mat4>> get_mat4_array(const StringName& property_name) const;
        void set_mat4_array(std::string_view property_name, std::span<const Mat4>);
        void set_mat4_array(const StringName& property_name, std::span<const Mat4>);

        std::optional<int32_t> get_int(std::string_view property_name) const;
        std::optional<int32_t> get_int(const StringName& property_name) const;
        void set_int(std::string_view property_name, int32_t);
        void set_int(const StringName& property_name, int32_t);

        std::optional<bool> get_bool(std::string_view property_name) const;
        std::optional<bool> get_bool(const StringName& property_name) const;
        void set_bool(std::string_view property_name, bool);
        void set_bool(const StringName& property_name, bool);

        std::optional<Texture2D> get_texture(std::string_view property_name) const;
        std::optional<Texture2D> get_texture(const StringName& property_name) const;
        void set_texture(std::string_view property_name, const Texture2D&);
        void set_texture(const StringName& property_name, const Texture2D&);

        std::optional<RenderTexture> get_render_texture(std::string_view property_name) const;
        std::optional<RenderTexture> get_render_texture(const StringName& property_name) const;
        void set_render_texture(std::string_view property_name, RenderTexture);
        void set_render_texture(const StringName& property_name, RenderTexture);

        std::optional<Cubemap> get_cubemap(std::string_view property_name) const;
        std::optional<Cubemap> get_cubemap(const StringName& property_name) const;
        void set_cubemap(std::string_view property_name, Cubemap);
        void set_cubemap(const StringName& property_name, Cubemap);

        void unset(std::string_view property_name);
        void unset(const StringName& property_name);

    private:
        friend bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
        friend std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
    std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
}
