#include "StationDefinedFrame.hpp"


osc::fd::StationDefinedFrame::StationDefinedFrame()
{
    constructProperty_ab_axis("+x");
    constructProperty_ab_x_ac_axis("+y");
}

const OpenSim::Frame& osc::fd::StationDefinedFrame::extendFindBaseFrame() const
{
    return getConnectee<OpenSim::Station>("point_a").getParentFrame().findBaseFrame();
}

SimTK::Transform osc::fd::StationDefinedFrame::extendFindTransformInBaseFrame() const
{
    // TODO:
    //
    // - compute transform from stations (put everything into the base frame)
    return {};  // TODO
}

void osc::fd::StationDefinedFrame::extendFinalizeFromProperties()
{
    // TODO: validate orthogonality of axis designations
}

void osc::fd::StationDefinedFrame::extendConnectToModel(OpenSim::Model&)
{
    // TODO: validate all three stations exist and are defined in the same base frame
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
