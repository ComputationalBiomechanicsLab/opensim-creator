#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/EdgePoints.hpp>

#include <OpenSim/Simulation/Model/ModelComponent.h>

namespace SimTK { class State; }

namespace osc::fd
{
    // virtual base class for an edge that starts at one location in ground and ends at
    // some other location in ground
    class FDVirtualEdge : public OpenSim::ModelComponent {
        OpenSim_DECLARE_ABSTRACT_OBJECT(FDVirtualEdge, ModelComponent)
    public:
        EdgePoints getEdgePointsInGround(SimTK::State const& state) const
        {
            return implGetEdgePointsInGround(state);
        }
    private:
        virtual EdgePoints implGetEdgePointsInGround(SimTK::State const&) const = 0;
    };
}
