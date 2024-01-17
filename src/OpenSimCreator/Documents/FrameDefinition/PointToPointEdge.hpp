#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/Edge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Point.h>

namespace osc::fd
{
    /**
     * A `PointToPointEdge` is an `Edge` that is defined between two other `Point`s in
     * the model. It is intended for creating named (and, when necessary, directional)
     * relationships between points in a model.
     *
     * Use Cases:
     *
     * Say your system needs to create a relationship between "the middle of the chest"
     * and "the middle of the pelvis", with `PointToPointEdge`, you could:
     *
     * - Define a `Marker` to the middle of the chest (a `Marker` is a `Point`)
     * - Define a `Marker` to the middle of the pelvis
     * - Define a `PointToPointEdge` between those two markers
     *
     * The resulting `PointToPointEdge` is an instance of a named `Edge` in the model, which
     * that means that it has `Output`s for its direction, magnitude, or start/end points.
     * This might be useful when (e.g.) you want to plot the above relationship as "the
     * incline of the torso" during a simulation.
     *
     * Further, `PointToPointEdge`s form part of a "Points and Edges" ecosystem, which can
     * be combined to create higher-level concepts. E.g.:
     *
     * - Combining two `Edge`s into a `CrossProductEdge` to create a plane normal
     * - Combining `Edge`s and `Point`s into a `CrossProductDefinedFrame` to define a new
     *   coordinate system
     */
    class PointToPointEdge final : public Edge {
        OpenSim_DECLARE_CONCRETE_OBJECT(PointToPointEdge, Edge)
    public:
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the edge's decorative arrow");

        OpenSim_DECLARE_SOCKET(first_point, OpenSim::Point, "The first point of the edge");
        OpenSim_DECLARE_SOCKET(second_point, OpenSim::Point, "The second point of the edge");

        PointToPointEdge();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;

    private:
        EdgePoints calcLocationsInGround(const SimTK::State&) const final;
    };
}
