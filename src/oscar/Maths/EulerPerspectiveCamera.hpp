#pragma once

#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Vec3.hpp>

namespace osc
{
    // camera that moves freely through space (e.g. FPS games)
    struct EulerPerspectiveCamera final {

        EulerPerspectiveCamera();

        Vec3 getFront() const;
        Vec3 getUp() const;
        Vec3 getRight() const;
        Mat4 getViewMtx() const;
        Mat4 getProjMtx(float aspectRatio) const;

        Vec3 origin;
        Radians pitch;
        Radians yaw;
        Radians verticalFOV;
        float znear;
        float zfar;
    };
}
