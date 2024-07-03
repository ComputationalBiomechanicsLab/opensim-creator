#include "IdentityFrameWarperFactory.h"

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <Simbody.h>

#include <memory>
#include <vector>

using namespace osc::mow;

std::unique_ptr<IFrameWarperFactory> osc::mow::IdentityFrameWarperFactory::implClone() const
{
    return std::make_unique<IdentityFrameWarperFactory>(*this);
}

std::vector<WarpDetail> osc::mow::IdentityFrameWarperFactory::implWarpDetails() const
{
    return {};
}

std::vector<ValidationCheckResult> osc::mow::IdentityFrameWarperFactory::implValidate(const WarpableModel&) const
{
    return {
        ValidationCheckResult{"this is an identity warp (i.e. it ignores warping this frame altogether)", ValidationCheckState::Warning},
    };
}

std::unique_ptr<IFrameWarper> osc::mow::IdentityFrameWarperFactory::implTryCreateFrameWarper(const WarpableModel&) const
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
