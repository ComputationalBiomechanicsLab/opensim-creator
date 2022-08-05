#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace osc
{
    class SceneRendererNewParams final {
    public:
        glm::ivec2 dimensions;
        int samples;
        bool wireframeMode;
        bool drawMeshNormals;
        bool drawRims;
        bool drawFloor;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::vec3 viewPos;
        glm::vec3 lightDirection;
        glm::vec3 lightColor;
        glm::vec4 backgroundColor;
        glm::vec4 rimColor;
        glm::vec3 floorLocation;
        float fixupScaleFactor;

        SceneRendererNewParams();
        SceneRendererNewParams(SceneRendererNewParams const&);
        SceneRendererNewParams(SceneRendererNewParams&&) noexcept;
        SceneRendererNewParams& operator=(SceneRendererNewParams const&);
        SceneRendererNewParams& operator=(SceneRendererNewParams&&) noexcept;
        ~SceneRendererNewParams() noexcept;
    };

    bool operator==(SceneRendererNewParams const&, SceneRendererNewParams const&);
    bool operator!=(SceneRendererNewParams const&, SceneRendererNewParams const&);
}