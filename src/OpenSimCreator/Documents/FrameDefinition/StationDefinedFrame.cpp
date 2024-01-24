#include "StationDefinedFrame.hpp"

#include <OpenSim/Common/Assertion.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <Simbody.h>

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>


namespace {
    // helper: returns the base frame that `station` is defined in
    OpenSim::Frame const& FindBaseFrame(OpenSim::Station const& station)
    {
        return station.getParentFrame().findBaseFrame();
    }

    // helper: returns the location of the `Station` w.r.t. its base frame
    SimTK::Vec3 GetLocationInBaseFrame(OpenSim::Station const& station)
    {
        return station.getParentFrame().findTransformInBaseFrame() * station.get_location();
    }

    // helper: tries to parse a given character as a designator for an axis of a 3D coordinate
    //
    // returns std::nullopt if the character cannot be parsed as an axis
    std::optional<SimTK::CoordinateAxis> TryParseAsCoordinateAxis(std::string_view::value_type c)
    {
        switch (c) {
        case 'x':
        case 'X':
            return SimTK::CoordinateAxis::XCoordinateAxis{};
        case 'y':
        case 'Y':
            return SimTK::CoordinateAxis::YCoordinateAxis{};
        case 'z':
        case 'Z':
            return SimTK::CoordinateAxis::ZCoordinateAxis{};
        default:
            return std::nullopt;
        }
    }

    // helper: tries to parse the given string as a potentially-signed representation of a
    // 3D coordinate dimension (e.g. "-x" --> SimTK::CoordinateDirection::NegXDirection()`)
    //
    // returns std::nullopt if the string does not have the required syntax
    std::optional<SimTK::CoordinateDirection> TryParseAsCoordinateDirection(std::string_view s)
    {
        if (s.empty())
        {
            return std::nullopt;  // cannot parse: input is empty
        }

        // handle and consume sign (direction) prefix (e.g. '+' / '-')
        const bool isNegated = s.front() == '-';
        if (isNegated || s.front() == '+')
        {
            s = s.substr(1);
        }

        if (s.empty())
        {
            return std::nullopt;  // cannot parse: the input was just a prefix with no axis (e.g. "+")
        }

        // handle axis suffix
        std::optional<SimTK::CoordinateAxis> const maybeAxis = TryParseAsCoordinateAxis(s.front());
        if (!maybeAxis)
        {
            return std::nullopt;
        }

        return SimTK::CoordinateDirection{*maybeAxis, isNegated ? -1 : 1};
    }

    // helper: tries to parse the string value held within `prop` as a coordinate direction, throwing
    // if the parse isn't possible
    SimTK::CoordinateDirection ParseAsCoordinateDirectionOrThrow(
        OpenSim::Component const& owner,
        OpenSim::Property<std::string> const& prop)
    {
        if (auto axis = TryParseAsCoordinateDirection(prop.getValue()))
        {
            return *axis;
        }
        else
        {
            std::stringstream ss;
            ss << prop.getName() << ": has an invalid value ('" << prop.getValue() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
            OPENSIM_THROW(OpenSim::Exception, owner, std::move(ss).str());
        }
    }
}


osc::fd::StationDefinedFrame::StationDefinedFrame()
{
    constructProperty_ab_axis("+x");
    constructProperty_ab_x_ac_axis("+y");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getPointA() const
{
    return getConnectee<OpenSim::Station>("point_a");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getPointB() const
{
    return getConnectee<OpenSim::Station>("point_b");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getPointC() const
{
    return getConnectee<OpenSim::Station>("point_c");
}

const OpenSim::Station& osc::fd::StationDefinedFrame::getOriginPoint() const
{
    return getConnectee<OpenSim::Station>("origin_point");
}

const OpenSim::Frame& osc::fd::StationDefinedFrame::extendFindBaseFrame() const
{
    return FindBaseFrame(getPointA());
}

SimTK::Transform osc::fd::StationDefinedFrame::extendFindTransformInBaseFrame() const
{
    return _transformInBaseFrame;
}

void osc::fd::StationDefinedFrame::extendFinalizeFromProperties()
{
    Super::extendFinalizeFromProperties();

    // parse `ab_axis`
    const SimTK::CoordinateDirection abDirection = ParseAsCoordinateDirectionOrThrow(*this, getProperty_ab_axis());

    // parse `ab_x_ac_axis`
    const SimTK::CoordinateDirection abXacDirection = ParseAsCoordinateDirectionOrThrow(*this, getProperty_ab_x_ac_axis());

    // ensure `ab_axis` is orthogonal to `ab_x_ac_axis`
    if (abDirection.hasSameAxis(abXacDirection)) {
        std::stringstream ss;
        ss << getProperty_ab_axis().getName() << " (" << getProperty_ab_axis().getValue() << ") and " << getProperty_ab_x_ac_axis().getName() << " (" << getProperty_ab_x_ac_axis().getValue() << ") are not orthogonal";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }

    // update vector-to-axis mappings so that `extendConnectToModel` knows how
    // computed vectors (e.g. `ab_x_ac_axis`) relate to the frame transform (e.g. +Y)
    _basisVectorToFrameMappings = {
        abDirection,
        abXacDirection,
        abDirection.crossProductAxis(abXacDirection),
    };
}

void osc::fd::StationDefinedFrame::extendConnectToModel(OpenSim::Model& model)
{
    Super::extendConnectToModel(model);

    // ensure all of the `Station`'s have the same base frame
    //
    // this is a hard requirement, because we need to know _for certain_ that
    // the relative transform of this frame doesn't change w.r.t. the base
    // frame during integration
    //
    // (e.g. it would cause mayhem if a Joint was defined using a
    // `StationDefinedFrame` that, itself, changes in response to a change in that
    // Joint's coordinates)
    OpenSim::Frame const& pointABaseFrame = FindBaseFrame(getPointA());
    OpenSim::Frame const& pointBBaseFrame = FindBaseFrame(getPointB());
    OpenSim::Frame const& pointCBaseFrame = FindBaseFrame(getPointC());
    OpenSim::Frame const& originPointFrame = FindBaseFrame(getOriginPoint());
    OPENSIM_ASSERT_FRMOBJ_ALWAYS(&pointABaseFrame == &pointBBaseFrame && "`point_b` is defined in a different base frame from `point_a`. All `Station`s (`point_a`, `point_b`, `point_c`, and `origin_point` of a `StationDefinedFrame` must be defined in the same base frame.");
    OPENSIM_ASSERT_FRMOBJ_ALWAYS(&pointABaseFrame == &pointCBaseFrame && "`point_c` is defined in a different base frame from `point_a`. All `Station`s (`point_a`, `point_b`, `point_c`, and `origin_point` of a `StationDefinedFrame` must be defined in the same base frame.");
    OPENSIM_ASSERT_FRMOBJ_ALWAYS(&pointABaseFrame == &originPointFrame && "`origin_point` is defined in a different base frame from `point_a`. All `Station`s (`point_a`, `point_b`, `point_c`, and `origin_point` of a `StationDefinedFrame` must be defined in the same base frame.");

    // once we know _for certain_ that all of the points can be calculated w.r.t.
    // the same base frame, we can precompute the transform
    _transformInBaseFrame = calcTransformInBaseFrame();
}

SimTK::Transform osc::fd::StationDefinedFrame::calcTransformInBaseFrame() const
{
    // get raw input data
    const SimTK::Vec3 posA = GetLocationInBaseFrame(getPointA());
    const SimTK::Vec3 posB = GetLocationInBaseFrame(getPointB());
    const SimTK::Vec3 posC = GetLocationInBaseFrame(getPointC());
    const SimTK::Vec3 originPoint = GetLocationInBaseFrame(getOriginPoint());

    // compute orthonormal basis vectors
    const SimTK::UnitVec3 ac(posC - posA);
    const SimTK::UnitVec3 ab(posB - posA);
    const SimTK::UnitVec3 ab_x_ac(cross(ab, ac));
    const SimTK::UnitVec3 ab_x_ab_x_ac(cross(ab, ab_x_ac));

    // remap them into a 3x3 "change of basis" matrix for each frame axis
    SimTK::Mat33 orientation{};
    orientation.col(_basisVectorToFrameMappings[0].getAxis()) = _basisVectorToFrameMappings[0].getDirection() * SimTK::Vec3(ab);
    orientation.col(_basisVectorToFrameMappings[1].getAxis()) = _basisVectorToFrameMappings[1].getDirection() * SimTK::Vec3(ab_x_ac);
    orientation.col(_basisVectorToFrameMappings[2].getAxis()) = _basisVectorToFrameMappings[2].getDirection() * SimTK::Vec3(ab_x_ab_x_ac);

    // combine with the origin point to create the complete transform in the base frame
    return SimTK::Transform{SimTK::Rotation{orientation}, originPoint};
}

SimTK::Transform osc::fd::StationDefinedFrame::calcTransformInGround(const SimTK::State& state) const
{
    return extendFindBaseFrame().getTransformInGround(state) * _transformInBaseFrame;
}

SimTK::SpatialVec osc::fd::StationDefinedFrame::calcVelocityInGround(const SimTK::State& state) const
{
    // note: this calculation is inspired from the one found in `OpenSim/Simulation/Model/OffsetFrame.h`

    const OpenSim::Frame& baseFrame = findBaseFrame();

    // get the (angular + linear) velocity of the base frame w.r.t. ground
    const SimTK::SpatialVec vbf = baseFrame.getVelocityInGround(state);

    // calculate the rigid _offset_ (not position) of this frame w.r.t. ground
    const SimTK::Vec3 offset = baseFrame.getTransformInGround(state).R() * findTransformInBaseFrame().p();

    return SimTK::SpatialVec{
        // the angular velocity of this frame is the same as its base frame (it's a rigid attachment)
        vbf(0),

        // the linear velocity of this frame is the linear velocity of its base frame, _plus_ the
        // rejection of this frame's offset from the base frame's angular velocity
        //
        // this is to account for the fact that rotation around the base frame will affect the linear
        // velocity of frames that are at an offset away from the rotation axis
        vbf(1) + (vbf(0) % offset),
    };
}

SimTK::SpatialVec osc::fd::StationDefinedFrame::calcAccelerationInGround(const SimTK::State& state) const
{
    // note: this calculation is inspired from the one found in `OpenSim/Simulation/Model/OffsetFrame.h`

    const OpenSim::Frame& baseFrame = findBaseFrame();

    // get the (angular + linear) velocity and acceleration of the base frame w.r.t. ground
    const SimTK::SpatialVec vbf = baseFrame.getVelocityInGround(state);
    const SimTK::SpatialVec abf = baseFrame.getAccelerationInGround(state);

    // calculate the rigid _offset_ (not position) of this frame w.r.t. ground
    const SimTK::Vec3 offset = baseFrame.getTransformInGround(state).R() * findTransformInBaseFrame().p();

    return SimTK::SpatialVec{
        // the angular acceleration of this frame is the same as its base frame (it's a rigid attachment)
        abf(0),

        // the linear acceleration of this frame is:
        //
        // - the linear acceleration of its base frame
        //
        // - plus the rejection of this frame's offset from from the base frame's angular velocity (to
        //   account for the fact that rotational acceleration in the base frame becomes linear acceleration
        //   for any frames attached at an offset that isn't along the rotation axis)
        //
        // - plus the rejection of TODO
        abf(1) + (abf(0) % offset) + (vbf(0) % (vbf(0) % offset)),
    };
}
