#include "PointToPointEdge.h"

#include <libopensimcreator/Documents/CustomComponents/EdgePoints.h>
#include <libopensimcreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>

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
    const EdgePoints coords = getLocationsInGround(state);

    appendOut.push_back(CreateDecorativeArrow(
        coords.start,
        coords.end,
        get_Appearance()
    ));
}

EdgePoints osc::fd::PointToPointEdge::calcLocationsInGround(const SimTK::State& state) const
{
    return {
        getConnectee<OpenSim::Point>("first_point").getLocationInGround(state),
        getConnectee<OpenSim::Point>("second_point").getLocationInGround(state),
    };
}
