#include "StationDefinedFrameWarperFactory.h"

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <Simbody.h>

#include <memory>
#include <vector>

using namespace osc::mow;

std::unique_ptr<IFrameWarperFactory> osc::mow::StationDefinedFrameWarperFactory::implClone() const
{
    return std::make_unique<StationDefinedFrameWarperFactory>(*this);
}

std::vector<WarpDetail> osc::mow::StationDefinedFrameWarperFactory::implWarpDetails() const
{
    return {};
}

std::vector<ValidationCheckResult> osc::mow::StationDefinedFrameWarperFactory::implValidate(const WarpableModel&) const
{
    return {
        ValidationCheckResult{"this frame is automatically warped when the model warper warps all stations in the model", ValidationCheckState::Ok},
    };
}

std::unique_ptr<IFrameWarper> osc::mow::StationDefinedFrameWarperFactory::implTryCreateFrameWarper(const WarpableModel&) const
{
    class IdentityFrameWarper final : public IFrameWarper {
    private:
        SimTK::Transform implWarp(const SimTK::Transform& transform) const override
        {
            return transform;  // identity
        }
    };
    return std::make_unique<IdentityFrameWarper>();
}
