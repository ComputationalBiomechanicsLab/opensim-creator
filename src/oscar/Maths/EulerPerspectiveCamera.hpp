#pragma once

#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Vec3.hpp>

namespace osc
{
    // camera that moves freely through space (e.g. FPS games)
    struct EulerPerspectiveCamera final {

        EulerPerspectiveCamera();

        Vec3 getFront() const noexcept;
        Vec3 getUp() const noexcept;
        Vec3 getRight() const noexcept;
        Mat4 getViewMtx() const noexcept;
        Mat4 getProjMtx(float aspectRatio) const noexcept;

        Vec3 origin;
        float pitch;
        float yaw;
        float fov;
        float znear;
        float zfar;
    };
}
