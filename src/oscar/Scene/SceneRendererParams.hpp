#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>

namespace osc
{
    struct SceneRendererParams final {

        static constexpr Color DefaultLightColor()
        {
            return {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f, 1.0f};
        }

        static constexpr Color DefaultBackgroundColor()
        {
            return {0.89f, 0.89f, 0.89f, 1.0f};
        }

        static constexpr Vec3 DefaultFloorLocation()
        {
            return {0.0f, -0.001f, 0.0f};
        }

        SceneRendererParams();

        Vec2i dimensions;
        AntiAliasingLevel antiAliasingLevel;
        bool drawMeshNormals;
        bool drawRims;
        bool drawShadows;
        bool drawFloor;
        float nearClippingPlane;
        float farClippingPlane;
        Mat4 viewMatrix;
        Mat4 projectionMatrix;
        Vec3 viewPos;
        Vec3 lightDirection;
        Color lightColor;  // ignores alpha
        float ambientStrength;
        float diffuseStrength;
        float specularStrength;
        float specularShininess;
        Color backgroundColor;
        Color rimColor;
        Vec2 rimThicknessInPixels;
        Vec3 floorLocation;
        float fixupScaleFactor;
    };

    bool operator==(SceneRendererParams const&, SceneRendererParams const&);
    bool operator!=(SceneRendererParams const&, SceneRendererParams const&);
}
