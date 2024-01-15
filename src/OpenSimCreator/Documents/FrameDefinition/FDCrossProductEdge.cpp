#include "FDCrossProductEdge.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>

#include <OpenSim/Simulation/Model/Appearance.h>
#include <Simbody.h>

using osc::fd::EdgePoints;

osc::fd::FDCrossProductEdge::FDCrossProductEdge()
{
    constructProperty_showPlane(false);
    constructProperty_Appearance(OpenSim::Appearance{});
    SetColorAndOpacity(upd_Appearance(), c_CrossProductEdgeDefaultColor);
}

void osc::fd::FDCrossProductEdge::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    EdgePoints const coords = getEdgePointsInGround(state);

    // draw edge
    appendOut.push_back(CreateDecorativeArrow(
        coords.start,
        coords.end,
        get_Appearance()
    ));

    // if requested, draw a parallelogram from the two edges
    if (get_showPlane())
    {
        auto const [aPoints, bPoints] = getBothEdgePoints(state);
        appendOut.push_back(CreateParallelogramMesh(
            coords.start,
            aPoints.end - aPoints.start,
            bPoints.end - bPoints.start,
            get_Appearance()
        ));
    }
}

std::pair<EdgePoints, EdgePoints> osc::fd::FDCrossProductEdge::getBothEdgePoints(SimTK::State const& state) const
{
    return
    {
        getConnectee<FDVirtualEdge>("edgeA").getEdgePointsInGround(state),
        getConnectee<FDVirtualEdge>("edgeB").getEdgePointsInGround(state),
    };
}

EdgePoints osc::fd::FDCrossProductEdge::implGetEdgePointsInGround(SimTK::State const& state) const
{
    std::pair<EdgePoints, EdgePoints> const edgePoints = getBothEdgePoints(state);
    return  CrossProduct(edgePoints.first, edgePoints.second);
}
