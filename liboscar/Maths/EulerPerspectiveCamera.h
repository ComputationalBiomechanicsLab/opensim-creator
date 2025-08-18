#pragma once

#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/Vec3.h>

namespace osc
{
    // a camera that moves freely through 3D space, oriented via pitch and yaw Euler
    // angles (no roll - i.e. as in FPS games)
    struct EulerPerspectiveCamera final {

        EulerPerspectiveCamera();

        Vec3 front() const;
        Vec3 up() const;
        Vec3 right() const;
        Matrix4x4 view_matrix() const;
        Matrix4x4 projection_matrix(float aspect_ratio) const;

        Vec3 origin = {};
        Radians pitch = Degrees{0};
        Radians yaw = Degrees{180};
        Radians vertical_field_of_view = Degrees{35};
        float znear = 0.1f;
        float zfar = 1000.0f;
    };
}
