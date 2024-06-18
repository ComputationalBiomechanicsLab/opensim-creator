#pragma once

#include <OpenSimCreator/Documents/CustomComponents/Edge.h>
#include <OpenSimCreator/Documents/CustomComponents/EdgePoints.h>

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>

#include <utility>

namespace OpenSim { class ModelDisplayHints; }

namespace osc::fd
{
    /**
     * A `CrossProductEdge` is an `Edge` that is calculated from the cross product
     * of two other `Edge`s. Specifically, it is an edge that is relative to, and
     * expressed in, ground in the following way:
     *
     * - It starts at `first_edge.start`
     * - It ends at `first_edge.start + (first_edge x second_edge)`
     * - It displays (via `generateDecorations`) as an arrow that starts at `first_edge.start`
     *   and ends at `first_edge.start + display_arrow_length*normalize(first_edge x second_edge)`
     *
     * The main utility of `CrossProductEdge` is to define orthogonal vectors and planes in
     * a model. For example, you could use `Marker`s, or `SphereLandmark`s to define points
     * of interest in your model (lowest-level), and then relate them to eachover using
     * `PointToPointEdge`s (mid-level), followed by establishing plane normals using a
     * `CrossProductEdge` (mid-to-high-level). Defining planes/normals this way is the
     * basis for biomechanical coordinate systems and custom motion metrics.
     *
     * Related: `CrossProductDefinedFrame` enables creating entire coordinate systems (`Frame`s),
     * rather than a single `Edge` (`CrossProductEdge`) from two `Edge`s.
     */
    class CrossProductEdge final : public Edge {
        OpenSim_DECLARE_CONCRETE_OBJECT(CrossProductEdge, Edge)
    public:
        OpenSim_DECLARE_PROPERTY(show_plane, bool, "Show/hide displaying a decorative plane formed from the two edges that were used to compute the cross product (decorative)");
        OpenSim_DECLARE_PROPERTY(arrow_display_length, double, "The length of the displayed cross-product arrow decoration (decorative)");
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the cross-product arrow decoration (decorative)");

        OpenSim_DECLARE_SOCKET(first_edge, Edge, "The first edge parameter for the cross product calculation");
        OpenSim_DECLARE_SOCKET(second_edge, Edge, "The second edge parameter for the cross product calculation");

        CrossProductEdge();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;

    private:
        EdgePoints calcLocationsInGround(const SimTK::State&) const final;

        std::pair<EdgePoints, EdgePoints> getBothEdgePoints(const SimTK::State&) const;
    };
}
