#pragma once

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Station.h>

namespace osc::fd
{
    /**
     * A `SphereLandmark` is a `Station` with customizable (decorative) `radius`
     * and `Appearance`. It is intended to help visualize (and place) points of interest
     * in a model.
     *
     * Example use-cases:
     *
     * - Fitting a `SphereLandmark` to part of a mesh (e.g. a femoral head) by editing the `radius`
     *   and visually fitting it
     *
     * - Color-coded landmarks for presentation, visual grouping, etc.
     */
    class SphereLandmark final : public OpenSim::Station {
        OpenSim_DECLARE_CONCRETE_OBJECT(SphereLandmark, OpenSim::Station)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the landmark's decorative sphere");
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance landmark's decorative sphere");

        SphereLandmark();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;
    };
}
