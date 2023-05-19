#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace osc
{
    // camera that moves freely through space (e.g. FPS games)
    struct EulerPerspectiveCamera final {

        EulerPerspectiveCamera();

        glm::vec3 getFront() const noexcept;
        glm::vec3 getUp() const noexcept;
        glm::vec3 getRight() const noexcept;
        glm::mat4 getViewMtx() const noexcept;
        glm::mat4 getProjMtx(float aspectRatio) const noexcept;

        glm::vec3 pos;
        float pitch;
        float yaw;
        float fov;
        float znear;
        float zfar;
    };
}
