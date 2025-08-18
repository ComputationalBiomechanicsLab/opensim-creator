#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/Quat.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>
#include <liboscar/Utils/Conversion.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/SmallMatrix.h>

#include <array>

namespace osc { struct Transform; }

namespace osc
{
    template<>
    struct Converter<Vec3, SimTK::Vec3> final {
        SimTK::Vec3 operator()(const Vec3&) const;
    };

    template<>
    struct Converter<EulerAngles, SimTK::Vec3> final {
        SimTK::Vec3 operator()(const EulerAngles&) const;
    };

    template<>
    struct Converter<Matrix3x3, SimTK::Mat33> final {
        SimTK::Mat33 operator()(const Matrix3x3&) const;
    };

    template<>
    struct Converter<Vec3, SimTK::Inertia> final {
        SimTK::Inertia operator()(const Vec3&) const;
    };

    template<>
    struct Converter<Transform, SimTK::Transform> final {
        SimTK::Transform operator()(const Transform&) const;
    };

    template<>
    struct Converter<Quat, SimTK::Rotation> final {
        SimTK::Rotation operator()(const Quat&) const;
    };

    template<>
    struct Converter<EulerAngles, SimTK::Rotation> final {
        SimTK::Rotation operator()(const EulerAngles&) const;
    };

    template<>
    struct Converter<Color, SimTK::Vec3> final {
        SimTK::Vec3 operator()(const Color&) const;
    };

    template<>
    struct Converter<SimTK::Vec3, Vec3> final {
        Vec3 operator()(const SimTK::Vec3&) const;
    };

    template<>
    struct Converter<SimTK::UnitVec3, Vec3> final {
        Vec3 operator()(const SimTK::UnitVec3&) const;
    };

    template<>
    struct Converter<SimTK::Transform, Matrix4x4> final {
        Matrix4x4 operator()(const SimTK::Transform&) const;
    };

    template<>
    struct Converter<SimTK::Mat33, Matrix3x3> final {
        Matrix3x3 operator()(const SimTK::Mat33&) const;
    };

    template<>
    struct Converter<SimTK::Rotation, Matrix4x4> final {
        Matrix4x4 operator()(const SimTK::Rotation&) const;
    };

    template<>
    struct Converter<SimTK::Rotation, Quat> final {
        Quat operator()(const SimTK::Rotation&) const;
    };

    template<>
    struct Converter<SimTK::Rotation, EulerAngles> final {
        EulerAngles operator()(const SimTK::Rotation&) const;
    };

    template<>
    struct Converter<SimTK::Vec6, std::array<float, 6>> final {
        std::array<float, 6> operator()(const SimTK::Vec6&) const;
    };

    template<>
    struct Converter<SimTK::Transform, Transform> final {
        Transform operator()(const SimTK::Transform&) const;
    };
}
