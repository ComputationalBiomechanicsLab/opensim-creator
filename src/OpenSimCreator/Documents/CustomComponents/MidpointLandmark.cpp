#include "MidpointLandmark.h"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>

#include <Simbody.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Point.h>

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
    const auto& [first, second] = lookupPoints();
    return 0.5*(first.getLocationInGround(state) + second.getLocationInGround(state));
}

SimTK::Vec3 osc::fd::MidpointLandmark::calcVelocityInGround(const SimTK::State& state) const
{
    const auto& [first, second] = lookupPoints();
    return 0.5*(first.getVelocityInGround(state) + second.getVelocityInGround(state));
}

SimTK::Vec3 osc::fd::MidpointLandmark::calcAccelerationInGround(const SimTK::State& state) const
{
    const auto& [first, second] = lookupPoints();
    return 0.5*(first.getAccelerationInGround(state), second.getAccelerationInGround(state));
}

std::pair<const OpenSim::Point&, const OpenSim::Point&> osc::fd::MidpointLandmark::lookupPoints() const
{
    return {
        getConnectee<OpenSim::Point>("first_point"),
        getConnectee<OpenSim::Point>("second_point"),
    };
}
