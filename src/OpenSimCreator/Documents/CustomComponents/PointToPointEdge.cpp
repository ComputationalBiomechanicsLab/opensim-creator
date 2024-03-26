#include "PointToPointEdge.h"

#include <OpenSimCreator/Documents/CustomComponents/EdgePoints.h>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>

using osc::fd::EdgePoints;

osc::fd::PointToPointEdge::PointToPointEdge()
{
    constructProperty_Appearance(OpenSim::Appearance{});
    SetColorAndOpacity(upd_Appearance(), c_PointToPointEdgeDefaultColor);
}

void osc::fd::PointToPointEdge::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    EdgePoints const coords = getLocationsInGround(state);

    appendOut.push_back(CreateDecorativeArrow(
        coords.start,
        coords.end,
        get_Appearance()
    ));
}

EdgePoints osc::fd::PointToPointEdge::calcLocationsInGround(SimTK::State const& state) const
{
    return {
        getConnectee<OpenSim::Point>("first_point").getLocationInGround(state),
        getConnectee<OpenSim::Point>("second_point").getLocationInGround(state),
    };
}
