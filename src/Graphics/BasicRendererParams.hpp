#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace osc
{
    class BasicRendererParams final {
    public:
        glm::vec3 lightDirection = {1.0f, 0.0f, 0.0f};
        glm::mat4 projectionMatrix{1.0f};
        glm::mat4 viewMatrix{1.0f};
        glm::vec3 viewPos = {0.0f, 0.0f, 0.0f};
        glm::vec3 lightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    };
}

