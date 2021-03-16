#pragma once

#include <SimTKcommon.h>

namespace osmv {
    [[nodiscard]] inline SimTK::Vec3 stk_vec3_from(float v[3]) noexcept {
        return {
            static_cast<double>(v[0]),
            static_cast<double>(v[1]),
            static_cast<double>(v[2]),
        };
    }

    [[nodiscard]] inline SimTK::Inertia stk_inertia_from(float v[3]) noexcept {
        return {
            static_cast<double>(v[0]),
            static_cast<double>(v[1]),
            static_cast<double>(v[2]),
        };
    }
}
