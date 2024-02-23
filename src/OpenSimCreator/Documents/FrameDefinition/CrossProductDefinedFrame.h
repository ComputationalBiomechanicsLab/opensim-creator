#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/Edge.h>
#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.h>

#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/Point.h>

#include <string>

namespace osc::fd
{
    /**
     * A `CrossProductDefinedFrame` is `Frame` that has its orientation computed from
     * the cross product of two other `Edge`s and its origin point located at a
     * specified `Point`.
     *
     * This is intended to be used as an alternative to `OffsetFrame` when a model
     * designer wants to explicitly establish coordinate systems from relationships
     * between `Edge`s/`Point`s in a model. This approach is in contrast to defining
     * those relationships implicitly (usually, with external software), and "baking"
     * the resulting orientation + origin into an `OffsetFrame`.
     *
     * Advantages:
     *
     * - The International Society of Biomechanics (ISB) defines biomechnical coordinate
     *   systems using the "Grood-Suntay" method, which uses similar approaches
     *   when establishing coordinate systems (doi: 10.1115/1.3138397).
     *
     * - It is (usually) easier to establish `Point`s of interest and `Edge`s in a model
     *   than is is to arbitrarily edit the Euler angles of an `OffsetFrame`. A
     *   `CrossProductDefinedFrame` directly integrates with `Point`-/`Edge`-based workflows.
     *
     * - A `CrossProductDefinedFrame` can easily be converted into an `OffsetFrame` The
     *   reverse is not true. If a model designer goes through the effort of establishing
     *   `Point`s/`Edge`s, a `CrossProductDefinedFrame` lets them explicitly encode the
     *   relationship into the model file itself.
     *
     * - Some algorithms (3D warping, scaling, etc.) work on locations in space, rather than
     *   on 3x3 matrices/quaternions. If you want to use one of those algorithms, you _must_
     *   define model relationships via `Point`s and `Edge`s - Gram-Schmidt only goes so far
     *   (trust me ;)).
     *
     * Disadvantages:
     *
     * - `CrossProductDefinedFrame`s cannot be manually oriented/positioned. You _must_ instead
     *   edit the `Edge`s (or, indirectly, the `Edge`'s `Point`s) or convert (one-way) the frame
     *   to an `OffsetFrame` if you want to do that.
     *
     * - Because `CrossProductDefinedFrame` is arbitrarily dependent on other components in
     *   the model (`Edge`s), there is a lot more potential for dependency-related errors.
     *   See `Error Cases` below.
     *
     * - Because `CrossProductDefinedFrame` is dependent on cross products, you must ensure that
     *   the chosen `Edge`s are definitely non-parallel. See `Error Cases` below.
     *
     * Error Cases:
     *
     * - `axis_edge` and `other_edge` must never be parallel. Cross products will not able to
     *   produce a sane coordinate system in this case. It is assumed that you have chosen
     *   two `Edge`s that are always non-parallel under all simulation conditions (e.g. they
     *   are both defined as stationary non-parallel edges on a mesh).
     *
     * - The `axis_edge_axis` and `first_cross_product_axis` must be orthogonal (e.g.
     *   'x' and '-z', not 'x' and '-x'). The implementation needs a minimum of two non-parallel
     *   edges and two orthogonal axes in order to compute the desired frame.
     *
     * - `axis_edge` and `other_edge` must never be defined on a `Frame` that is a "child" (e.g.
     *   via a `Joint`, or `OffsetFrame`) of the `CrossProductDefinedFrame`. Doing this creates
     *   a cyclic dependency and is definitely an error. E.g.:
     *
     *   - `parent_frame` (`CrossProductDefinedFrame`) depends on `axis_edge`
     *   - `axis_edge` depends on `child_frame`
     *   - `child_frame`, via its ground-transform, depends on `parent_frame`
     *   - ... which depends on `axis_edge` - uh oh
     *
     * Details:
     *
     * The name `CrossProductDefinedFrame` refers to the fact that two of the axes are defined
     * via cross products. This design ensures that the resulting frame axes are orthogonal to
     * eachover (assuming your edges never point in the same direction ;)) - and it mirrors
     * best practises from biomechnical standards.
     *
     * The nomenclature "axis"-edge and "other"-edge refers to the fact that `axis_edge` is
     * an `Edge` that directly becomes an axis of the resulting `Frame`, whereas `other_edge`
     * is an `Edge` that is only used to seed the first cross-product. The "first cross product"
     * nomenclature alludes to the order of operations/assignments: "axis", then "first product",
     * then "second product".
     */
    class CrossProductDefinedFrame final : public OpenSim::PhysicalFrame {
        OpenSim_DECLARE_CONCRETE_OBJECT(CrossProductDefinedFrame, PhysicalFrame)
    public:
        OpenSim_DECLARE_PROPERTY(axis_edge_axis, std::string, "The resulting frame axis that `axis_edge` points in the direction of. Can be -x, +x, -y, +y, -z, or +z");
        OpenSim_DECLARE_PROPERTY(first_cross_product_axis, std::string, "The resulting frame axis that `axis_edge x other_edge` points in the direction of. Can be -x, +x, -y, +y, -z, or +z, but must be orthogonal to `axis_edge_axis`");
        OpenSim_DECLARE_PROPERTY(force_showing_frame, bool, "Forcibly show/hide the resulting frame's decoration - even if `show_frames` is enabled in the model's display hints (decorative)");

        OpenSim_DECLARE_SOCKET(axis_edge, Edge, "The edge that determines the direction of the resulting frame's `axis_edge_axis`");
        OpenSim_DECLARE_SOCKET(other_edge, Edge, "An edge that is cross-producted with `axis_edge` to create the edge that determines the direction of the resulting frame's `first_cross_product_axis`");
        OpenSim_DECLARE_SOCKET(origin, OpenSim::Point, "The point that determines where the resulting frame's origin point is located");

        CrossProductDefinedFrame();

    private:
        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>&
        ) const final;

        void extendFinalizeFromProperties() final;

        struct ParsedAxisArguments final {
            MaybeNegatedAxis axisEdge;
            MaybeNegatedAxis otherEdge;
        };
        ParsedAxisArguments tryParseAxisArgumentsAsOrthogonalAxes() const;

        SimTK::Transform calcTransformInGround(SimTK::State const&) const final;
        SimTK::SpatialVec calcVelocityInGround(SimTK::State const&) const final;
        SimTK::SpatialVec calcAccelerationInGround(SimTK::State const&) const final;
        void extendAddToSystem(SimTK::MultibodySystem&) const final;
    };
}
