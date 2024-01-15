#include "FDPointToPointEdge.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>

using osc::fd::EdgePoints;

osc::fd::FDPointToPointEdge::FDPointToPointEdge()
{
    constructProperty_Appearance(OpenSim::Appearance{});
    SetColorAndOpacity(upd_Appearance(), c_PointToPointEdgeDefaultColor);
}

void osc::fd::FDPointToPointEdge::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    EdgePoints const coords = getEdgePointsInGround(state);

    appendOut.push_back(CreateDecorativeArrow(
        coords.start,
        coords.end,
        get_Appearance()
    ));
}

EdgePoints osc::fd::FDPointToPointEdge::implGetEdgePointsInGround(SimTK::State const& state) const
{
    auto const& pointA = getConnectee<OpenSim::Point>("pointA");
    SimTK::Vec3 const pointAGroundLoc = pointA.getLocationInGround(state);

    auto const& pointB = getConnectee<OpenSim::Point>("pointB");
    SimTK::Vec3 const pointBGroundLoc = pointB.getLocationInGround(state);

    return {pointAGroundLoc, pointBGroundLoc};
}
