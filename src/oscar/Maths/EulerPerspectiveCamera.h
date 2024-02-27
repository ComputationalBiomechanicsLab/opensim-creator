#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec3.h>

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

        Vec3 origin = {};
        Radians pitch = Degrees{0};
        Radians yaw = Degrees{180};
        Radians verticalFOV = Degrees{35};
        float znear = 0.1f;
        float zfar = 1000.0f;
    };
}
