#pragma once

#include <OpenSimCreator/Documents/CustomComponents/EdgePoints.h>

#include <Simbody.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>

#include <utility>

namespace osc::fd
{
    /**
     * An `Edge` is an OpenSim abstraction for a pair (start, end) of locations in ground.
     * Edges are intended to define relationships between points in 3D space, such that their
     * relative separation or the direction between them can be computed on-the-fly.
     *
     * `Edge`s are the dual of `Point`. Like `Point`s, they don't prescibe how their start or
     * end locations are computed, or which frame they are defined in. The motivation behind
     * this design is to keep the definition loose: `Edge` could mean "the vector between
     * two `Point`s" (see: `PointToPointEdge`), or it could mean "the cross-product between
     * two other `Edge`s" (see: `CrossProductEdge`).
     *
     * Use Cases:
     *
     * Say your system wants to establish the relationship between a point at the top of
     * a mesh and a point at the bottom of a mesh as "the Y axis of the femur". You could
     * use a `PointToPointEdge` to explicitly define that relationship. The resulting `Edge`
     * could then be used to (e.g.) compute how the resulting axis moves in ground during
     * a simulation, or composed into other `Edge`-driven components (e.g. `CrossProductEdge`,
     * `CrossProductDefinedFrame`).
     */
    class Edge : public OpenSim::ModelComponent {
        OpenSim_DECLARE_ABSTRACT_OBJECT(Edge, ModelComponent)
    public:
        OpenSim_DECLARE_OUTPUT(start_location, SimTK::Vec3, getStartLocationInGround, SimTK::Stage::Position)
        OpenSim_DECLARE_OUTPUT(end_location, SimTK::Vec3, getEndLocationInGround, SimTK::Stage::Position)
        OpenSim_DECLARE_OUTPUT(direction, SimTK::Vec3, getDirectionInGround, SimTK::Stage::Position)
        OpenSim_DECLARE_OUTPUT(length, double, getLengthInGround, SimTK::Stage::Position)

        /**
         * Get the start and end locations of the edge relative to, and expressed in,
         * ground. Only valid when supplied a `SimTK::State` at `SimTK::Stage::Position`
         * or higher.
         */
        const EdgePoints& getLocationsInGround(const SimTK::State&) const;

        /**
         * Get the start location of the edge relative to, and expressed in, ground. Only
         * valid when supplied a `SimTK::State` at `SimTK::Stage::Position` or higher.
         */
        SimTK::Vec3 getStartLocationInGround(const SimTK::State&) const;

        /**
        * Get the end location of the edge relative to, and expressed in, ground. Only
        * valid when supplied a `SimTK::State` at `SimTK::Stage::Position` or higher.
        */
        SimTK::Vec3 getEndLocationInGround(const SimTK::State&) const;

        /**
         * Get the direction of the edge expressed in ground. Equivalent to calculating
         * `normalize(end - start)`. Only valid when supplied a `SimTK::State` at
         * `SimTK::Stage::Position` or higher.
         */
        SimTK::Vec3 getDirectionInGround(const SimTK::State&) const;

        /**
         * Get the length (magnitude) of the vector formed between the start and end
         * location in ground. Only valid when supplied a `SimTK::State` at
         * `SimTK::Stage::Position` or higher.
         */
        double getLengthInGround(const SimTK::State&) const;

    protected:
        /**
         * `Edge` extension method: concrete `Edge` implementations override this.
         *
         * Calculate the start and end locations of this `Edge` relative to, and expressed
         * in, ground. Implementations can expect the provided `SimTK::State` to be realized
         * to at least `SimTK::Stage::Position`.
         */
        virtual EdgePoints calcLocationsInGround(const SimTK::State&) const = 0;

        void extendAddToSystem(SimTK::MultibodySystem&) const override;

    private:
        mutable CacheVariable<EdgePoints> _locationsCV;
    };
}
