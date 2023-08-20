#pragma once

#include "oscar/Graphics/AntiAliasingLevel.hpp"
#include "oscar/Graphics/Color.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

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

        static constexpr glm::vec3 DefaultFloorLocation()
        {
            return {0.0f, -0.001f, 0.0f};
        }

        SceneRendererParams();

        glm::ivec2 dimensions;
        AntiAliasingLevel samples;
        bool drawMeshNormals;
        bool drawRims;
        bool drawShadows;
        bool drawFloor;
        float nearClippingPlane;
        float farClippingPlane;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::vec3 viewPos;
        glm::vec3 lightDirection;
        Color lightColor;  // ignores alpha
        float ambientStrength;
        float diffuseStrength;
        float specularStrength;
        float specularShininess;
        Color backgroundColor;
        Color rimColor;
        glm::vec2 rimThicknessInPixels;
        glm::vec3 floorLocation;
        float fixupScaleFactor;
    };

    bool operator==(SceneRendererParams const&, SceneRendererParams const&);
    bool operator!=(SceneRendererParams const&, SceneRendererParams const&);
}
