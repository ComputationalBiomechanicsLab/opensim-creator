#include "SphereLandmark.h"

#include <libopensimcreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>

#include <OpenSim/Simulation/Model/Appearance.h>

osc::fd::SphereLandmark::SphereLandmark()
{
    constructProperty_radius(c_SphereDefaultRadius);
    constructProperty_Appearance(OpenSim::Appearance{});
    SetColorAndOpacity(upd_Appearance(), c_SphereDefaultColor);
}

void osc::fd::SphereLandmark::generateDecorations(
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
