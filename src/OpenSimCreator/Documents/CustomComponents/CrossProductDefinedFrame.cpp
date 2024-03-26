#include "CrossProductDefinedFrame.h"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>

#include <Simbody.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelVisualPreferences.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <oscar/Maths/CoordinateDirection.h>

#include <array>
#include <optional>
#include <sstream>
#include <utility>

using osc::fd::CrossProductDefinedFrame;

osc::fd::CrossProductDefinedFrame::CrossProductDefinedFrame()
{
    constructProperty_axis_edge_axis("+x");
    constructProperty_first_cross_product_axis("+y");
    constructProperty_force_showing_frame(true);
}

void osc::fd::CrossProductDefinedFrame::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    if (get_force_showing_frame() ||
        getModel().get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames())
    {
        appendOut.push_back(CreateDecorativeFrame(
            getTransformInGround(state)
        ));
    }
}

void osc::fd::CrossProductDefinedFrame::extendFinalizeFromProperties()
{
    OpenSim::PhysicalFrame::extendFinalizeFromProperties();  // call parent
    tryParseAxisArgumentsAsOrthogonalAxes();  // throws on error
}

CrossProductDefinedFrame::ParsedAxisArguments osc::fd::CrossProductDefinedFrame::tryParseAxisArgumentsAsOrthogonalAxes() const
{
    // ensure `axis_edge_axis` is a correct property value
    auto const maybeAxisEdge = CoordinateDirection::try_parse(get_axis_edge_axis());
    if (not maybeAxisEdge) {
        std::stringstream ss;
        ss << getProperty_axis_edge_axis().getName() << ": has an invalid value ('" << get_axis_edge_axis() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }
    CoordinateDirection const& axisEdge = *maybeAxisEdge;

    // ensure `first_cross_product_axis` is a correct property value
    auto const maybeOtherEdge = CoordinateDirection::try_parse(get_first_cross_product_axis());
    if (not maybeOtherEdge) {
        std::stringstream ss;
        ss << getProperty_first_cross_product_axis().getName() << ": has an invalid value ('" << get_first_cross_product_axis() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }
    CoordinateDirection const& otherEdge = *maybeOtherEdge;

    // ensure `axis_edge_axis` is an orthogonal axis to `other_edge_axis`
    if (axisEdge.axis() == otherEdge.axis()) {
        std::stringstream ss;
        ss << getProperty_axis_edge_axis().getName() << " (" << get_axis_edge_axis() << ") and " << getProperty_first_cross_product_axis().getName() << " (" << get_first_cross_product_axis() << ") are not orthogonal";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }

    return ParsedAxisArguments{axisEdge, otherEdge};
}

SimTK::Transform osc::fd::CrossProductDefinedFrame::calcTransformInGround(SimTK::State const& state) const
{
    // parse axis properties
    auto const [axisEdge, otherEdge] = tryParseAxisArgumentsAsOrthogonalAxes();

    // get other edges/points via sockets
    SimTK::UnitVec3 const axisEdgeDir =
        CalcDirection(getConnectee<Edge>("axis_edge").getLocationsInGround(state));
    SimTK::UnitVec3 const otherEdgeDir =
        CalcDirection(getConnectee<Edge>("other_edge").getLocationsInGround(state));
    SimTK::Vec3 const originLocationInGround =
        getConnectee<OpenSim::Point>("origin").getLocationInGround(state);

    // this is what the algorithm must ultimately compute in order to
    // calculate a change-of-basis (rotation) matrix
    std::array<SimTK::UnitVec3, 3> axes{};

    // assign first axis
    SimTK::UnitVec3& firstAxisDir = axes.at(axisEdge.axis().index());
    firstAxisDir = axisEdge.is_negated() ? -axisEdgeDir : axisEdgeDir;

    // compute second axis (via cross product)
    SimTK::UnitVec3& secondAxisDir = axes.at(otherEdge.axis().index());
    {
        secondAxisDir = SimTK::UnitVec3{SimTK::cross(axisEdgeDir, otherEdgeDir)};
        if (otherEdge.is_negated()) {
            secondAxisDir = -secondAxisDir;
        }
    }

    // compute third axis (via cross product)
    {
        // care: the user is allowed to specify axes out-of-order
        //
        // so this bit of code calculates the correct ordering, assuming that
        // axes are in a circular X -> Y -> Z relationship w.r.t. cross products
        struct ThirdEdgeOperands final
        {
            SimTK::UnitVec3 const& firstDir;
            SimTK::UnitVec3 const& secondDir;
            CoordinateAxis resultAxis;
        };
        ThirdEdgeOperands const ops = axisEdge.axis().next() == otherEdge.axis() ?
            ThirdEdgeOperands{firstAxisDir, secondAxisDir, otherEdge.axis().next()} :
            ThirdEdgeOperands{secondAxisDir, firstAxisDir, axisEdge.axis().next()};

        SimTK::UnitVec3 const thirdAxisDir = SimTK::UnitVec3{SimTK::cross(ops.firstDir, ops.secondDir)};
        axes.at(ops.resultAxis.index()) = thirdAxisDir;
    }

    // create transform from orthogonal axes and origin
    SimTK::Mat33 const rotationMatrix{SimTK::Vec3{axes[0]}, SimTK::Vec3{axes[1]}, SimTK::Vec3{axes[2]}};
    SimTK::Rotation const rotation{rotationMatrix};

    return SimTK::Transform{rotation, originLocationInGround};
}

SimTK::SpatialVec osc::fd::CrossProductDefinedFrame::calcVelocityInGround(SimTK::State const&) const
{
    return {};  // TODO: see OffsetFrame::calcVelocityInGround
}

SimTK::SpatialVec osc::fd::CrossProductDefinedFrame::calcAccelerationInGround(SimTK::State const&) const
{
    return {};  // TODO: see OffsetFrame::calcAccelerationInGround
}

void osc::fd::CrossProductDefinedFrame::extendAddToSystem(SimTK::MultibodySystem& system) const
{
    OpenSim::PhysicalFrame::extendAddToSystem(system);  // call parent
    setMobilizedBodyIndex(getModel().getGround().getMobilizedBodyIndex()); // TODO: the frame must be associated to a mobod
}
