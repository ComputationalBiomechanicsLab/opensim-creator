#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    struct SceneRendererParams final {

        static constexpr Color defaultLightColor()
        {
            return {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        }

        static constexpr Color defaultBackgroundColor()
        {
            return {0.89f, 0.89f, 0.89f};
        }

        static constexpr Vec3 defaultFloorLocation()
        {
            return {0.0f, -0.001f, 0.0f};
        }

        SceneRendererParams();

        friend bool operator==(const SceneRendererParams&, const SceneRendererParams&) = default;

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
}
