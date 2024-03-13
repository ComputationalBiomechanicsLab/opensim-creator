#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    // a camera that moves freely through 3D space, oriented via Euler angles (e.g. as used in FPS games)
    struct EulerPerspectiveCamera final {

        EulerPerspectiveCamera();

        Vec3 front() const;
        Vec3 up() const;
        Vec3 right() const;
        Mat4 view_matrix() const;
        Mat4 projection_matrix(float aspectRatio) const;

        Vec3 origin = {};
        Radians pitch = Degrees{0};
        Radians yaw = Degrees{180};
        Radians vertical_fov = Degrees{35};
        float znear = 0.1f;
        float zfar = 1000.0f;
    };
}
