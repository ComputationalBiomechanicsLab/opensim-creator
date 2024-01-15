#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/FDVirtualEdge.hpp>
#include <OpenSimCreator/Documents/FrameDefinition/MaybeNegatedAxis.hpp>

#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/Point.h>

#include <string>

namespace osc::fd
{
    // a frame that is defined by:
    //
    // - an "axis" edge
    // - a designation of what axis the "axis" edge lies along
    // - an "other" edge, which should be non-parallel to the "axis" edge
    // - a desgination of what axis the cross product `axis x other` lies along
    // - an "origin" point, which is where the origin of the frame should be defined
    class LandmarkDefinedFrame final : public OpenSim::PhysicalFrame {
        OpenSim_DECLARE_CONCRETE_OBJECT(LandmarkDefinedFrame, PhysicalFrame)
    public:
        OpenSim_DECLARE_SOCKET(axisEdge, FDVirtualEdge, "The edge from which to create the first axis");
        OpenSim_DECLARE_SOCKET(otherEdge, FDVirtualEdge, "Some other edge that is non-parallel to `axisEdge` and can be used (via a cross product) to define the frame");
        OpenSim_DECLARE_SOCKET(origin, OpenSim::Point, "The origin (position) of the frame");
        OpenSim_DECLARE_PROPERTY(axisEdgeDimension, std::string, "The dimension to assign to `axisEdge`. Can be -x, +x, -y, +y, -z, or +z");
        OpenSim_DECLARE_PROPERTY(secondAxisDimension, std::string, "The dimension to assign to the second axis that is generated from the cross-product of `axisEdge` with `otherEdge`. Can be -x, +x, -y, +y, -z, or +z and must be orthogonal to `axisEdgeDimension`");
        OpenSim_DECLARE_PROPERTY(forceShowingFrame, bool, "Whether to forcibly show the frame's decoration, even if showing frames is disabled at the model-level (decorative)");

        LandmarkDefinedFrame();

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
