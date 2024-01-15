#include "LandmarkDefinedFrame.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>

#include <OpenSim/Common/Exception.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelVisualPreferences.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <Simbody.h>

#include <array>
#include <optional>
#include <sstream>
#include <utility>

using osc::fd::LandmarkDefinedFrame;

osc::fd::LandmarkDefinedFrame::LandmarkDefinedFrame()
{
    constructProperty_axisEdgeDimension("+x");
    constructProperty_secondAxisDimension("+y");
    constructProperty_forceShowingFrame(true);
}

void osc::fd::LandmarkDefinedFrame::generateDecorations(
    bool,
    const OpenSim::ModelDisplayHints&,
    const SimTK::State& state,
    SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const
{
    if (get_forceShowingFrame() ||
        getModel().get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames())
    {
        appendOut.push_back(CreateDecorativeFrame(
            getTransformInGround(state)
        ));
    }
}

void osc::fd::LandmarkDefinedFrame::extendFinalizeFromProperties()
{
    OpenSim::PhysicalFrame::extendFinalizeFromProperties();  // call parent
    tryParseAxisArgumentsAsOrthogonalAxes();  // throws on error
}

LandmarkDefinedFrame::ParsedAxisArguments osc::fd::LandmarkDefinedFrame::tryParseAxisArgumentsAsOrthogonalAxes() const
{
    // ensure `axisEdge` is a correct property value
    std::optional<MaybeNegatedAxis> const maybeAxisEdge = ParseAxisDimension(get_axisEdgeDimension());
    if (!maybeAxisEdge)
    {
        std::stringstream ss;
        ss << getProperty_axisEdgeDimension().getName() << ": has an invalid value ('" << get_axisEdgeDimension() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }
    MaybeNegatedAxis const& axisEdge = *maybeAxisEdge;

    // ensure `otherEdge` is a correct property value
    std::optional<MaybeNegatedAxis> const maybeOtherEdge = ParseAxisDimension(get_secondAxisDimension());
    if (!maybeOtherEdge)
    {
        std::stringstream ss;
        ss << getProperty_secondAxisDimension().getName() << ": has an invalid value ('" << get_secondAxisDimension() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }
    MaybeNegatedAxis const& otherEdge = *maybeOtherEdge;

    // ensure `axisEdge` is orthogonal to `otherEdge`
    if (!IsOrthogonal(axisEdge, otherEdge))
    {
        std::stringstream ss;
        ss << getProperty_axisEdgeDimension().getName() << " (" << get_axisEdgeDimension() << ") and " << getProperty_secondAxisDimension().getName() << " (" << get_secondAxisDimension() << ") are not orthogonal";
        OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
    }

    return ParsedAxisArguments{axisEdge, otherEdge};
}

SimTK::Transform osc::fd::LandmarkDefinedFrame::calcTransformInGround(SimTK::State const& state) const
{
    // parse axis properties
    auto const [axisEdge, otherEdge] = tryParseAxisArgumentsAsOrthogonalAxes();

    // get other edges/points via sockets
    SimTK::UnitVec3 const axisEdgeDir =
        CalcDirection(getConnectee<FDVirtualEdge>("axisEdge").getEdgePointsInGround(state));
    SimTK::UnitVec3 const otherEdgeDir =
        CalcDirection(getConnectee<FDVirtualEdge>("otherEdge").getEdgePointsInGround(state));
    SimTK::Vec3 const originLocationInGround =
        getConnectee<OpenSim::Point>("origin").getLocationInGround(state);

    // this is what the algorithm must ultimately compute in order to
    // calculate a change-of-basis (rotation) matrix
    std::array<SimTK::UnitVec3, 3> axes{};
    static_assert(axes.size() == osc::NumOptions<AxisIndex>());

    // assign first axis
    SimTK::UnitVec3& firstAxisDir = axes.at(ToIndex(axisEdge.axisIndex));
    firstAxisDir = axisEdge.isNegated ? -axisEdgeDir : axisEdgeDir;

    // compute second axis (via cross product)
    SimTK::UnitVec3& secondAxisDir = axes.at(ToIndex(otherEdge.axisIndex));
    {
        secondAxisDir = SimTK::UnitVec3{SimTK::cross(axisEdgeDir, otherEdgeDir)};
        if (otherEdge.isNegated)
        {
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
            AxisIndex resultAxisIndex;
        };
        ThirdEdgeOperands const ops = Next(axisEdge.axisIndex) == otherEdge.axisIndex ?
            ThirdEdgeOperands{firstAxisDir, secondAxisDir, Next(otherEdge.axisIndex)} :
            ThirdEdgeOperands{secondAxisDir, firstAxisDir, Next(axisEdge.axisIndex)};

        SimTK::UnitVec3 const thirdAxisDir = SimTK::UnitVec3{SimTK::cross(ops.firstDir, ops.secondDir)};
        axes.at(ToIndex(ops.resultAxisIndex)) = thirdAxisDir;
    }

    // create transform from orthogonal axes and origin
    SimTK::Mat33 const rotationMatrix{SimTK::Vec3{axes[0]}, SimTK::Vec3{axes[1]}, SimTK::Vec3{axes[2]}};
    SimTK::Rotation const rotation{rotationMatrix};

    return SimTK::Transform{rotation, originLocationInGround};
}

SimTK::SpatialVec osc::fd::LandmarkDefinedFrame::calcVelocityInGround(SimTK::State const&) const
{
    return {};  // TODO: see OffsetFrame::calcVelocityInGround
}

SimTK::SpatialVec osc::fd::LandmarkDefinedFrame::calcAccelerationInGround(SimTK::State const&) const
{
    return {};  // TODO: see OffsetFrame::calcAccelerationInGround
}

void osc::fd::LandmarkDefinedFrame::extendAddToSystem(SimTK::MultibodySystem& system) const
{
    OpenSim::PhysicalFrame::extendAddToSystem(system);  // call parent
    setMobilizedBodyIndex(getModel().getGround().getMobilizedBodyIndex()); // TODO: the frame must be associated to a mobod
}
