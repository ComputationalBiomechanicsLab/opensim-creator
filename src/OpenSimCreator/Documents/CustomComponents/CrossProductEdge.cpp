#include "CrossProductEdge.h"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>

#include <Simbody.h>
#include <OpenSim/Simulation/Model/Appearance.h>

using osc::fd::EdgePoints;

osc::fd::CrossProductEdge::CrossProductEdge()
{
    constructProperty_show_plane(false);
    constructProperty_arrow_display_length(1.0);
    constructProperty_Appearance(OpenSim::Appearance{});

    SetColorAndOpacity(upd_Appearance(), c_CrossProductEdgeDefaultColor);
}

void osc::fd::CrossProductEdge::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    EdgePoints const coords = getLocationsInGround(state);

    // draw edge
    appendOut.push_back(CreateDecorativeArrow(
        coords.start,
        coords.end,
        get_Appearance()
    ));

    // if requested, draw a parallelogram from the two edges
    if (get_show_plane())
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

EdgePoints osc::fd::CrossProductEdge::calcLocationsInGround(const SimTK::State& state) const
{
    const auto& [first, second] = getBothEdgePoints(state);
    return CrossProduct(first, second);  // TODO: sort out magnitude etc.
}

std::pair<EdgePoints, EdgePoints> osc::fd::CrossProductEdge::getBothEdgePoints(const SimTK::State& state) const
{
    return {
        getConnectee<Edge>("first_edge").getLocationsInGround(state),
        getConnectee<Edge>("second_edge").getLocationsInGround(state),
    };
}
