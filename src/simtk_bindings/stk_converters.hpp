#pragma once

#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <SimTKcommon.h>

namespace osc {

    // convert 3 packed floats to a SimTK::Vec3
    [[nodiscard]] inline SimTK::Vec3 stk_vec3_from(float v[3]) noexcept {
        return {
            static_cast<double>(v[0]),
            static_cast<double>(v[1]),
            static_cast<double>(v[2]),
        };
    }

    // convert a glm::vec3 to a SimTK::Vec3
    [[nodiscard]] inline SimTK::Vec3 stk_vec3_from(glm::vec3 const& v) noexcept {
        return {
            static_cast<double>(v.x),
            static_cast<double>(v.y),
            static_cast<double>(v.z),
        };
    }

    // convert 3 packed floats to a SimTK::Inertia
    [[nodiscard]] inline SimTK::Inertia stk_inertia_from(float v[3]) noexcept {
        return {
            static_cast<double>(v[0]),
            static_cast<double>(v[1]),
            static_cast<double>(v[2]),
        };
    }

    [[nodiscard]] inline glm::vec3 stk_vec3_from_Vec3(SimTK::Vec3 const& v) noexcept {
        return glm::vec3(v[0], v[1], v[2]);
    }

    [[nodiscard]] inline glm::vec4 stk_vec4_from_Vec3(SimTK::Vec3 const& v, float w = 1.0f) noexcept {
        return glm::vec4{v[0], v[1], v[2], w};
    }

    // convert a SimTK::Transform to a glm::mat4x3
    [[nodiscard]] inline glm::mat4x3 stk_mat4x3_from_xform(SimTK::Transform const& t) noexcept {
        // glm::mat4x3 is column major:
        //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
        //     (and just Google "glm column major?")
        //
        // SimTK is row-major, carefully read the sourcecode for
        // `SimTK::Transform`.

        glm::mat4x3 m;

        // x0 y0 z0 w0
        SimTK::Rotation const& r = t.R();
        SimTK::Vec3 const& p = t.p();

        {
            auto const& row0 = r[0];
            m[0][0] = static_cast<float>(row0[0]);
            m[1][0] = static_cast<float>(row0[1]);
            m[2][0] = static_cast<float>(row0[2]);
            m[3][0] = static_cast<float>(p[0]);
        }

        {
            auto const& row1 = r[1];
            m[0][1] = static_cast<float>(row1[0]);
            m[1][1] = static_cast<float>(row1[1]);
            m[2][1] = static_cast<float>(row1[2]);
            m[3][1] = static_cast<float>(p[1]);
        }

        {
            auto const& row2 = r[2];
            m[0][2] = static_cast<float>(row2[0]);
            m[1][2] = static_cast<float>(row2[1]);
            m[2][2] = static_cast<float>(row2[2]);
            m[3][2] = static_cast<float>(p[2]);
        }

        return m;
    }

    [[nodiscard]] inline glm::mat4x3 stk_mat4x4_from_xform(SimTK::Transform const& t) noexcept {
        return glm::mat4{stk_mat4x3_from_xform(t)};
    }

    [[nodiscard]] inline SimTK::Transform stk_xform_from_mat4x3(glm::mat4x3 const& m) noexcept {
        // glm::mat4 is column-major, SimTK::Transform is effectively
        // row-major

        SimTK::Mat33 mtx{
            m[0][0], m[1][0], m[2][0],
            m[0][1], m[1][1], m[2][1],
            m[0][2], m[1][2], m[2][2],
        };
        SimTK::Vec3 translation{m[3][0], m[3][1], m[3][2]};

        SimTK::Rotation rot{mtx};

        return SimTK::Transform{rot, translation};
    }
}
