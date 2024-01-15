#include "MidpointLandmark.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>

#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <Simbody.h>

osc::fd::MidpointLandmark::MidpointLandmark()
{
    constructProperty_radius(c_SphereDefaultRadius);
    constructProperty_Appearance(OpenSim::Appearance{});
    SetColorAndOpacity(upd_Appearance(), c_MidpointDefaultColor);
}

void osc::fd::MidpointLandmark::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    appendOut.push_back(CreateDecorativeSphere(
        get_radius(),
        getLocationInGround(state),
        get_Appearance()
    ));
}

SimTK::Vec3 osc::fd::MidpointLandmark::calcLocationInGround(const SimTK::State& state) const
{
    SimTK::Vec3 const pointALocation = getConnectee<OpenSim::Point>("pointA").getLocationInGround(state);
    SimTK::Vec3 const pointBLocation = getConnectee<OpenSim::Point>("pointB").getLocationInGround(state);
    return 0.5*(pointALocation + pointBLocation);
}

SimTK::Vec3 osc::fd::MidpointLandmark::calcVelocityInGround(const SimTK::State& state) const
{
    SimTK::Vec3 const pointAVelocity = getConnectee<OpenSim::Point>("pointA").getVelocityInGround(state);
    SimTK::Vec3 const pointBVelocity = getConnectee<OpenSim::Point>("pointB").getVelocityInGround(state);
    return 0.5*(pointAVelocity + pointBVelocity);
}

SimTK::Vec3 osc::fd::MidpointLandmark::calcAccelerationInGround(const SimTK::State& state) const
{
    SimTK::Vec3 const pointAAcceleration = getConnectee<OpenSim::Point>("pointA").getAccelerationInGround(state);
    SimTK::Vec3 const pointBAcceleration = getConnectee<OpenSim::Point>("pointB").getAccelerationInGround(state);
    return 0.5*(pointAAcceleration + pointBAcceleration);
}
