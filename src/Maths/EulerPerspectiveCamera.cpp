#include "EulerPerspectiveCamera.hpp"

#include "src/Maths/Constants.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>


osc::EulerPerspectiveCamera::EulerPerspectiveCamera() :
    pos{0.0f, 0.0f, 0.0f},
    pitch{0.0f},
    yaw{-fpi/2.0f},
    fov{fpi * 70.0f/180.0f},
    znear{0.1f},
    zfar{1000.0f}
{
}

glm::vec3 osc::EulerPerspectiveCamera::getFront() const noexcept
{
    return glm::normalize(glm::vec3
    {
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch),
    });
}

glm::vec3 osc::EulerPerspectiveCamera::getUp() const noexcept
{
    return glm::vec3{0.0f, 1.0f, 0.0f};
}

glm::vec3 osc::EulerPerspectiveCamera::getRight() const noexcept
{
    return glm::normalize(glm::cross(getFront(), getUp()));
}

glm::mat4 osc::EulerPerspectiveCamera::getViewMtx() const noexcept
{
    return glm::lookAt(pos, pos + getFront(), getUp());
}

glm::mat4 osc::EulerPerspectiveCamera::getProjMtx(float aspect_ratio) const noexcept
{
    return glm::perspective(fov, aspect_ratio, znear, zfar);
}
