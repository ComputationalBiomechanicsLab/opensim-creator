#pragma once

namespace osc
{
    // hold the distance along the depth (z) axis, in worldspace units, between a
    // camera's origin and each clipping plane
    struct CameraClippingPlanes final {
        friend constexpr bool operator==(const CameraClippingPlanes&, const CameraClippingPlanes&) = default;

        float znear = 1.0f;
        float zfar = -1.0f;
    };
}
