#include "FrameWarperFactories.h"

#include <OpenSimCreator/Documents/ModelWarper/IdentityFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>

#include <OpenSim/Simulation/Model/Model.h>

osc::mow::FrameWarperFactories::FrameWarperFactories(
    std::filesystem::path const&,
    OpenSim::Model const& model,
    ModelWarpConfiguration const& config)
{
    for (OpenSim::Frame const& frame : model.getComponentList<OpenSim::Frame>()) {
        // TODO: should actually look up the correct warping alg. etc.
        if (config.getShouldDefaultMissingFrameWarpsToIdentity()) {
            m_AbsPathToWarpLUT.try_emplace(frame.getAbsolutePathString(), std::make_unique<IdentityFrameWarperFactory>());
        }
    }
}
