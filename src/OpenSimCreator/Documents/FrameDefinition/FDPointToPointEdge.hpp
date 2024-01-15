#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FDVirtualEdge.hpp>

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Point.h>

namespace osc::fd
{
    // an edge that starts at virtual `pointA` and ends at virtual `pointB`
    class FDPointToPointEdge final : public FDVirtualEdge {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDPointToPointEdge, FDVirtualEdge)
    public:
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the edge (decorative)");
        OpenSim_DECLARE_SOCKET(pointA, OpenSim::Point, "The first point that the edge is connected to");
        OpenSim_DECLARE_SOCKET(pointB, OpenSim::Point, "The second point that the edge is connected to");

        FDPointToPointEdge();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;

    private:
        EdgePoints implGetEdgePointsInGround(SimTK::State const&) const final;
    };
}
