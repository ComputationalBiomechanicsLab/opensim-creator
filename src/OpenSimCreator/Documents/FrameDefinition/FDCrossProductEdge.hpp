#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/FDVirtualEdge.hpp>

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>

#include <utility>

namespace OpenSim { class ModelDisplayHints; }

namespace osc::fd
{
    // an edge that is computed from `edgeA x edgeB`
    //
    // - originates at `a.start`
    // - points in the direction of `a x b`
    // - has a magnitude of min(|a|, |b|) - handy for rendering
    class FDCrossProductEdge final : public FDVirtualEdge {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDCrossProductEdge, FDVirtualEdge)
    public:
        OpenSim_DECLARE_PROPERTY(showPlane, bool, "Whether to show the plane of the two edges the cross product was created from (decorative)");
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the edge (decorative)");
        OpenSim_DECLARE_SOCKET(edgeA, FDVirtualEdge, "The first edge parameter to the cross product calculation");
        OpenSim_DECLARE_SOCKET(edgeB, FDVirtualEdge, "The second edge parameter to the cross product calculation");

        FDCrossProductEdge();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;

    private:
        std::pair<EdgePoints, EdgePoints> getBothEdgePoints(SimTK::State const&) const;
        EdgePoints implGetEdgePointsInGround(SimTK::State const&) const final;
    };
}
