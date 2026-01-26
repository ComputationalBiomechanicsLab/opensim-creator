#pragma once

#include <liboscar/graphics/color.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/matrix3x3.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/maths/vector4.h>
#include <liboscar/utilities/conversion.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/SmallMatrix.h>

#include <array>

namespace osc { struct Transform; }

template<>
struct osc::Converter<osc::Vector3, SimTK::Vec3> final {
    SimTK::Vec3 operator()(const osc::Vector3&) const;
};

template<>
struct osc::Converter<osc::Vector3, SimTK::fVec3> final {
    SimTK::fVec3 operator()(const osc::Vector3&) const;
};

template<>
struct osc::Converter<osc::EulerAngles, SimTK::Vec3> final {
    SimTK::Vec3 operator()(const osc::EulerAngles&) const;
};

template<>
struct osc::Converter<osc::Matrix3x3, SimTK::Mat33> final {
    SimTK::Mat33 operator()(const osc::Matrix3x3&) const;
};

template<>
struct osc::Converter<osc::Vector3, SimTK::Inertia> final {
    SimTK::Inertia operator()(const osc::Vector3&) const;
};

template<>
struct osc::Converter<osc::Transform, SimTK::Transform> final {
    SimTK::Transform operator()(const osc::Transform&) const;
};

template<>
struct osc::Converter<osc::Quaternion, SimTK::Rotation> final {
    SimTK::Rotation operator()(const osc::Quaternion&) const;
};

template<>
struct osc::Converter<osc::EulerAngles, SimTK::Rotation> final {
    SimTK::Rotation operator()(const osc::EulerAngles&) const;
};

template<>
struct osc::Converter<osc::Color, SimTK::Vec3> final {
    SimTK::Vec3 operator()(const osc::Color&) const;
};

template<>
struct osc::Converter<SimTK::Vec3, osc::Vector3> final {
    osc::Vector3 operator()(const SimTK::Vec3&) const;
};

template<>
struct osc::Converter<SimTK::fVec3, osc::Vector3> final {
    osc::Vector3 operator()(const SimTK::fVec3&) const;
};

template<>
struct osc::Converter<SimTK::UnitVec3, osc::Vector3> final {
    osc::Vector3 operator()(const SimTK::UnitVec3&) const;
};

template<>
struct osc::Converter<SimTK::Transform, osc::Matrix4x4> final {
    osc::Matrix4x4 operator()(const SimTK::Transform&) const;
};

template<>
struct osc::Converter<SimTK::Mat33, osc::Matrix3x3> final {
    osc::Matrix3x3 operator()(const SimTK::Mat33&) const;
};

template<>
struct osc::Converter<SimTK::Rotation, osc::Matrix4x4> final {
    osc::Matrix4x4 operator()(const SimTK::Rotation&) const;
};

template<>
struct osc::Converter<SimTK::Rotation, osc::Quaternion> final {
    osc::Quaternion operator()(const SimTK::Rotation&) const;
};

template<>
struct osc::Converter<SimTK::Rotation, osc::EulerAngles> final {
    osc::EulerAngles operator()(const SimTK::Rotation&) const;
};

template<>
struct osc::Converter<SimTK::Vec6, std::array<float, 6>> final {
    std::array<float, 6> operator()(const SimTK::Vec6&) const;
};

template<>
struct osc::Converter<SimTK::Transform, osc::Transform> final {
    osc::Transform operator()(const SimTK::Transform&) const;
};
