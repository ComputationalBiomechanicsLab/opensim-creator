#pragma once

#include <Simbody.h>  // TODO: OpenSim's Appearance header doesn't include simbody properly
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Point.h>

#include <utility>

namespace OpenSim { class ModelDisplayHints; }

namespace osc::fd
{
    /**
    * A `MidpointLandmark` is a `Point` computed from the average of two other
    * `Point`s with a customizable (decorative) `radius` and `Appearance`. It
    * is intended to be used as part of model-building, where the requirements
    * such as "the midpoint between two condyls" can appear
    * (e.g. doi: 10.1016/s0021-9290(01)00222-6).
    */
    class MidpointLandmark final : public OpenSim::Point {
        OpenSim_DECLARE_CONCRETE_OBJECT(MidpointLandmark, OpenSim::Point)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the midpoint (decorative)")
        OpenSim_DECLARE_PROPERTY(Appearance, OpenSim::Appearance, "The appearance of the midpoint (decorative)")

        OpenSim_DECLARE_SOCKET(first_point, OpenSim::Point, "The first point that the midpoint lies between")
        OpenSim_DECLARE_SOCKET(second_point, OpenSim::Point, "The second point that the midpoint lies between")

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

        std::pair<OpenSim::Point const&, OpenSim::Point const&> lookupPoints() const;
    };
}
