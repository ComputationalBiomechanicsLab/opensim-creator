#pragma once

#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <Simbody.h>

#include <array>
#include <string>

namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace osc::fd
{
    /**
     * A `StationDefinedFrame` is a `Frame` that has its orientation and origin point computed
     * from `Station`s.
     *
     * Specifically, it is a `Frame` that is defined by:
     *
     * - Creating a triangle from `point_a`, `point_b`, and `point_c`
     * - Computing `ab_axis` as the unit vector `normalize(point_b - point_a)`
     * - Computing `ab_x_ac_axis` as the unit vector `normalize((point_b - point_a) x (point_c - point_a))`
     * - Computing the remaining axis as the unit vector `normalize((point_b - point_a) x ((point_b - point_a) x (point_c - point_a)))`
     * - Computing the resulting `Frame`'s `orientation` from the three resulting vectors, and its `position` from `frame_origin`
     * - The base frame of the resulting frame is equal to the base frame of all provided points, which must be equal
     *
     * `StationDefinedFrame` is intended to be used as a higher-level alternative to
     * `OffsetFrame` when a model designer is able to explicitly establish coordinate systems
     * from relationships between `Station`s in the model. This can be useful for
     * "landmark-driven" frame definition, and is in contrast to defining frames
     * implicitly (e.g. with external software, by eye) followed by baking that
     * implicit knowledge into an `OffsetFrame` as an `orientation` and `position`.
     *
     * Advantages
     * ==========
     *
     * - More closely matches the "Grood-Suntay" method of frame definition, which is (e.g.)
     *   how The International Society of Biomechanics (ISB) defines biomechnical coordinate
     *   systems (e.g., doi: 10.1115/1.3138397).
     *
     * - It is typically easier for model builders to establish `Station`s in their model from
     *   (e.g.) landmarks and relate them, rather than arbitrarily editing the Euler angles of an
     *   `OffsetFrame`.
     *
     * - Some algorithms (3D warping, scaling, etc.) operate on spatial locations, rather than
     *   on 3x3 matrices, quaternions, or Euler angles. If you want to use one of those
     *   algorithms to transform a model without resorting to tricks like Gram-Schmidt, you
     *   _must_ use a point-driven frame definition.
     *
     * - The way that `StationDefinedFrame` is formulated means that it can be automatically
     *   converted into an `OffsetFrame` with no loss of information.
     *
     * Disadvantages
     * =============
     *
     * - It requires that you can identify at least three points that form a triangle. Some models may
     *   not have three convenient "landmarks" that can be used in this way.
     *
     * - `StationDefinedFrame` isn't as directly customizable as an `OffsetFrame`. If you want to
     *   reorient the frame, you will have to edit the underlying `Station`s, or first perform
     *   a one-way conversion of the `StationDefinedFrame` to an `OffsetFrame`, or (better) add
     *   a child `OffsetFrame` to the `StationDefinedFrame`.
     *
     * Error Cases
     * ===========
     *
     * - The four points (three triangle points, one origin point) must be rigidly positioned in
     *   the same base frame. This is so that a state-independent rigid Frame can be defined from them.
     *
     * - The three triangle points must actually form a Triangle. Co-located, or parallel, points are
     *   an error.
     */
    class StationDefinedFrame final : public OpenSim::PhysicalFrame {
        OpenSim_DECLARE_CONCRETE_OBJECT(StationDefinedFrame, OpenSim::PhysicalFrame);
    public:
        OpenSim_DECLARE_PROPERTY(ab_axis, std::string, "The frame axis that points in the direction of `point_b - point_a`. Can be `-x`, `+x`, `-y`, `+y`, `-z`, or `+z`. Must be orthogonal to `ab_x_ac_axis`.");
        OpenSim_DECLARE_PROPERTY(ab_x_ac_axis, std::string, "The frame axis that points in the direction of `(point_b - point_a) x (point_c - point_a)`. Can be `-x`, `+x`, `-y`, `+y`, `-z`, or `+z`. Must be orthogonal to `ab_axis`.");

        OpenSim_DECLARE_SOCKET(point_a, OpenSim::Station, "Point `a` of a triangle that defines the frame's orientation. Must form a triangle with `point_b` and `point_c`. Note: `point_a`, `point_b`, `point_c`, and `frame_origin` must all share the same base frame.");
        OpenSim_DECLARE_SOCKET(point_b, OpenSim::Station, "Point `b` of a triangle that defines the frame's orientation. Must form a triangle with `point_a` and `point_c`. Note: `point_a`, `point_b`, `point_c`, and `frame_origin` must all share the same base frame.");
        OpenSim_DECLARE_SOCKET(point_c, OpenSim::Station, "Point `c` of a triangle that defines the frame's orientation. Must form a triangle with `point_a` and `point_b`. Note: `point_a`, `point_b`, `point_c`, and `frame_origin` must all share the same base frame.");
        OpenSim_DECLARE_SOCKET(origin_point, OpenSim::Station, "Point that defines the frame's origin point. Can be one of the triangle points. Note: `point_a`, `point_b`, `point_c`, and `frame_origin` must all share the same base frame.");

        StationDefinedFrame();

    private:
        const OpenSim::Station& getPointA() const;
        const OpenSim::Station& getPointB() const;
        const OpenSim::Station& getPointC() const;
        const OpenSim::Station& getOriginPoint() const;

        const OpenSim::Frame& extendFindBaseFrame() const final;
        SimTK::Transform extendFindTransformInBaseFrame() const final;
        void extendFinalizeFromProperties() final;
        void extendConnectToModel(OpenSim::Model&) final;

        SimTK::Transform calcTransformInGround(const SimTK::State&) const final;
        SimTK::SpatialVec calcVelocityInGround(const SimTK::State&) const final;
        SimTK::SpatialVec calcAccelerationInGround(const SimTK::State&) const;

        // determines how each orthonormal basis vector (`ab`, `ab x ac`, `ab x (ab x ac)`)
        // maps onto a frame axis index (x, 0; y, 1; z, 2)
        //
        // updated during `extendFinalizeProperties` (the mapping is dictated by the `ab_axis`
        // and `ab_x_ac_axis` properties)
        std::array<int, 3> _vectorToAxisIndexMappings = {0, 1, 2};
    };
}
