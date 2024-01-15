#pragma once

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Point.h>

namespace OpenSim { class ModelDisplayHints; }

namespace osc::fd
{
    // a landmark defined as a point between two other points
    class MidpointLandmark final : public OpenSim::Point {
        OpenSim_DECLARE_CONCRETE_OBJECT(MidpointLandmark, OpenSim::Point)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the midpoint (decorative)");
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the midpoint (decorative)");
        OpenSim_DECLARE_SOCKET(pointA, OpenSim::Point, "The first point that the midpoint is between");
        OpenSim_DECLARE_SOCKET(pointB, OpenSim::Point, "The second point that the midpoint is between");

        MidpointLandmark();

        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut
        ) const final;

    private:
        SimTK::Vec3 calcLocationInGround(const SimTK::State&) const final;
        SimTK::Vec3 calcVelocityInGround(const SimTK::State&) const final;
        SimTK::Vec3 calcAccelerationInGround(const SimTK::State&) const final;
    };
}
