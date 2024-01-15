#pragma once

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Station.h>

namespace osc::fd
{
    // a sphere landmark, where the center of the sphere is the point of interest
    class SphereLandmark final : public OpenSim::Station {
        OpenSim_DECLARE_CONCRETE_OBJECT(SphereLandmark, OpenSim::Station)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the sphere (decorative)");
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the sphere (decorative)");

        SphereLandmark();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;
    };
}
