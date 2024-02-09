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

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    class Material final {
    public:
        explicit Material(Shader);

        Shader const& getShader() const;

        // note: this differs from merely setting a vec4, because it is assumed
        // that the provided color is in sRGB and needs to be converted to a
        // linear color in the shader
        std::optional<Color> getColor(std::string_view propertyName) const;
        void setColor(std::string_view propertyName, Color const&);

        std::optional<std::span<Color const>> getColorArray(std::string_view propertyName) const;
        void setColorArray(std::string_view propertyName, std::span<Color const>);

        std::optional<float> getFloat(std::string_view propertyName) const;
        void setFloat(std::string_view propertyName, float);

        std::optional<std::span<float const>> getFloatArray(std::string_view propertyName) const;
        void setFloatArray(std::string_view propertyName, std::span<float const>);

        std::optional<Vec2> getVec2(std::string_view propertyName) const;
        void setVec2(std::string_view propertyName, Vec2);

        std::optional<Vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, Vec3);

        std::optional<std::span<Vec3 const>> getVec3Array(std::string_view propertyName) const;
        void setVec3Array(std::string_view propertyName, std::span<Vec3 const>);

        std::optional<Vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, Vec4);

        std::optional<Mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, Mat3 const&);

        std::optional<Mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, Mat4 const&);

        std::optional<std::span<Mat4 const>> getMat4Array(std::string_view propertyName) const;
        void setMat4Array(std::string_view propertyName, std::span<Mat4 const>);

        std::optional<int32_t> getInt(std::string_view propertyName) const;
        void setInt(std::string_view propertyName, int32_t);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view propertyName, Texture2D);
        void clearTexture(std::string_view propertyName);

        std::optional<RenderTexture> getRenderTexture(std::string_view propertyName) const;
        void setRenderTexture(std::string_view propertyName, RenderTexture);
        void clearRenderTexture(std::string_view propertyName);

        std::optional<Cubemap> getCubemap(std::string_view propertyName) const;
        void setCubemap(std::string_view propertyName, Cubemap);
        void clearCubemap(std::string_view propertyName);

        bool getTransparent() const;
        void setTransparent(bool);

        bool getDepthTested() const;
        void setDepthTested(bool);

        DepthFunction getDepthFunction() const;
        void setDepthFunction(DepthFunction);

        bool getWireframeMode() const;
        void setWireframeMode(bool);

        CullMode getCullMode() const;
        void setCullMode(CullMode);

        friend void swap(Material& a, Material& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(Material const&, Material const&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, Material const&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, Material const&);
}
