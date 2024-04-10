#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string_view>

// note: implementation is in `GraphicsImplementation.cpp`
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

        // note: this differs from merely setting a vec4, because it is assumed
        // that the provided color is in sRGB and needs to be converted to a
        // linear color in the shader
        std::optional<Color> get_color(std::string_view property_name) const;
        void set_color(std::string_view property_name, const Color&);

        std::optional<float> get_float(std::string_view property_name) const;
        void set_float(std::string_view property_name, float);

        std::optional<Vec3> get_vec3(std::string_view property_name) const;
        void set_vec3(std::string_view property_name, Vec3);

        std::optional<Vec4> get_vec4(std::string_view property_name) const;
        void set_vec4(std::string_view property_name, Vec4);

        std::optional<Mat3> get_mat3(std::string_view property_name) const;
        void set_mat3(std::string_view property_name, const Mat3&);

        std::optional<Mat4> get_mat4(std::string_view property_name) const;
        void set_mat4(std::string_view property_name, const Mat4&);

        std::optional<int32_t> get_int(std::string_view property_name) const;
        void set_int(std::string_view, int32_t);

        std::optional<bool> get_bool(std::string_view property_name) const;
        void set_bool(std::string_view property_name, bool);

        std::optional<Texture2D> get_texture(std::string_view property_name) const;
        void set_texture(std::string_view, Texture2D);

        friend void swap(MaterialPropertyBlock& a, MaterialPropertyBlock& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
        friend std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
    std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
}
