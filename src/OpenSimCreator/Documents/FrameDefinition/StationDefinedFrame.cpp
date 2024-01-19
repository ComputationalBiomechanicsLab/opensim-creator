#include "StationDefinedFrame.hpp"


namespace {
    OpenSim::Frame const& FindBaseFrame(OpenSim::Station const& station)
    {
        return station.getParentFrame().findBaseFrame();
    }

    SimTK::Vec3 GetLocationInBaseFrame(OpenSim::Station const& station)
    {
        return station.getParentFrame().findTransformInBaseFrame() * station.get_location();
    }
}


osc::fd::StationDefinedFrame::StationDefinedFrame()
{
    constructProperty_ab_axis("+x");
    constructProperty_ab_x_ac_axis("+y");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getPointA() const
{
    return getConnectee<OpenSim::Station>("point_a");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getPointB() const
{
    return getConnectee<OpenSim::Station>("point_b");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getPointC() const
{
    return getConnectee<OpenSim::Station>("point_c");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getOriginPoint() const
{
    return getConnectee<OpenSim::Station>("origin_point");
}

const OpenSim::Frame& osc::fd::StationDefinedFrame::extendFindBaseFrame() const
{
    return FindBaseFrame(getPointA());
}

SimTK::Transform osc::fd::StationDefinedFrame::extendFindTransformInBaseFrame() const
{
    // TODO: this should be hidden behind a model-stage cache variable

    // get raw input data
    const SimTK::Vec3 posA = GetLocationInBaseFrame(getPointA());
    const SimTK::Vec3 posB = GetLocationInBaseFrame(getPointB());
    const SimTK::Vec3 posC = GetLocationInBaseFrame(getPointC());
    const SimTK::Vec3 originPoint = GetLocationInBaseFrame(getOriginPoint());

    // compute orthonormal basis vectors
    const SimTK::UnitVec3 ac(posC - posA);
    const SimTK::UnitVec3 ab(posB - posA);
    const SimTK::UnitVec3 ab_x_ac(cross(ab, ac));
    const SimTK::UnitVec3 ab_x_ab_x_ac(cross(ab, ab_x_ac));

    // remap them into a 3x3 "change of basis" matrix for each frame axis
    SimTK::Mat33 orientation{};
    orientation.col(_vectorToAxisIndexMappings[0]) = SimTK::Vec3(ab);
    orientation.col(_vectorToAxisIndexMappings[1]) = SimTK::Vec3(ab_x_ac);
    orientation.col(_vectorToAxisIndexMappings[2]) = SimTK::Vec3(ab_x_ab_x_ac);

    // combine with the origin point to create the complete transform in the base frame
    return SimTK::Transform{SimTK::Rotation{orientation}, originPoint};
}

void osc::fd::StationDefinedFrame::extendFinalizeFromProperties()
{
    // TODO:
    //
    // - validate that `ab_axis` and `ab_x_ac_axis` can be parsed as an axis
    // - validate that `ab_axis` and `ab_x_ac_axis` are orthogonal
    // - compute `_vectorToAxisIndexMappings`
}

void osc::fd::StationDefinedFrame::extendConnectToModel(OpenSim::Model&)
{
    // TODO:
    //
    // - validate that all referred-to `Station`s have the same base frame
}

SimTK::Transform osc::fd::StationDefinedFrame::calcTransformInGround(const SimTK::State&) const
{
    // TODO: get base frame's transform in ground and compose it with `getTransformInBaseFrame`
    return {};
}

SimTK::SpatialVec osc::fd::StationDefinedFrame::calcVelocityInGround(const SimTK::State&) const
{
    // TODO: get base frame's velocity in ground and compose it with `getTransformInBaseFrame`
    //
    // see: `OpenSim::OffsetFrame<T>::calcVelocityInGround` for calculation
    return {};
}

SimTK::SpatialVec osc::fd::StationDefinedFrame::calcAccelerationInGround(const SimTK::State&) const
{
    // TODO: get base frame's acceleration in ground and compose it with `getTransformInBaseFrame`
    //
    // see: `OpenSim::OffsetFrame<T>::calcAccelerationInGround` for calculation
    return {};
}
