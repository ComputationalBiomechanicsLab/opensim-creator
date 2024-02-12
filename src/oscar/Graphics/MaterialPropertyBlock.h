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
    // than using a different material every time)
    class MaterialPropertyBlock final {
    public:
        MaterialPropertyBlock();

        void clear();
        bool isEmpty() const;

        // note: this differs from merely setting a vec4, because it is assumed
        // that the provided color is in sRGB and needs to be converted to a
        // linear color in the shader
        std::optional<Color> getColor(std::string_view propertyName) const;
        void setColor(std::string_view propertyName, Color const&);

        std::optional<float> getFloat(std::string_view propertyName) const;
        void setFloat(std::string_view propertyName, float);

        std::optional<Vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, Vec3);

        std::optional<Vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, Vec4);

        std::optional<Mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, Mat3 const&);

        std::optional<Mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, Mat4 const&);

        std::optional<int32_t> getInt(std::string_view propertyName) const;
        void setInt(std::string_view, int32_t);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view, Texture2D);

        friend void swap(MaterialPropertyBlock& a, MaterialPropertyBlock& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);
}
