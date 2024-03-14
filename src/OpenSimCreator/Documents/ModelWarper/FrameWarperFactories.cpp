#include "FrameWarperFactories.h"

#include <OpenSimCreator/Documents/ModelWarper/IdentityFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/StationDefinedFrameWarperFactory.h>

#include <OpenSim/Simulation/Model/StationDefinedFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Model.h>

osc::mow::FrameWarperFactories::FrameWarperFactories(
    std::filesystem::path const&,
    OpenSim::Model const& model,
    ModelWarpConfiguration const& config)
{
    // `StationDefinedFrame`s don't need a warper (they are warp-able by construction), but populated
    // the lookup with a named warper so the engine knows it's fine
    for (auto const& sdf : model.getComponentList<OpenSim::StationDefinedFrame>()) {
        m_AbsPathToWarpLUT.try_emplace(sdf.getAbsolutePathString(), std::make_unique<StationDefinedFrameWarperFactory>());
    }

    // if the configuration says "just identity-transform all unaccounted-for frames" then install
    // an identity warper for each unaccounted-for frame
    //
    // the identity warper should warn the user that this is happening though (it's incorrect to
    // entirely ignore warping, but useful for getting things going)
    if (config.getShouldDefaultMissingFrameWarpsToIdentity()) {
        for (auto const& pof : model.getComponentList<OpenSim::PhysicalOffsetFrame>()) {
            m_AbsPathToWarpLUT.try_emplace(pof.getAbsolutePathString(), std::make_unique<IdentityFrameWarperFactory>());
        }
    }
}
