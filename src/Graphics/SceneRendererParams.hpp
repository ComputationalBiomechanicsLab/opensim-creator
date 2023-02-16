#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace osc
{
    struct SceneRendererParams final {

        SceneRendererParams();

        glm::ivec2 dimensions;
        int32_t samples;
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
        glm::vec3 lightColor;
        float ambientStrength;
        float diffuseStrength;
        float specularStrength;
        float shininess;
        glm::vec4 backgroundColor;
        glm::vec4 rimColor;
        glm::vec2 rimThicknessInPixels;
        glm::vec3 floorLocation;
        float fixupScaleFactor;
    };

    bool operator==(SceneRendererParams const&, SceneRendererParams const&);
    bool operator!=(SceneRendererParams const&, SceneRendererParams const&);
}