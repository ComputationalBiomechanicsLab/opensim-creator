#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace osc
{
    class SceneRendererParams final {
    public:
        glm::ivec2 dimensions;
        int samples;
        bool drawMeshNormals;
        bool drawRims;
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
        glm::vec3 floorLocation;
        float fixupScaleFactor;

        SceneRendererParams();
        SceneRendererParams(SceneRendererParams const&);
        SceneRendererParams(SceneRendererParams&&) noexcept;
        SceneRendererParams& operator=(SceneRendererParams const&);
        SceneRendererParams& operator=(SceneRendererParams&&) noexcept;
        ~SceneRendererParams() noexcept;
    };

    bool operator==(SceneRendererParams const&, SceneRendererParams const&);
    bool operator!=(SceneRendererParams const&, SceneRendererParams const&);
}