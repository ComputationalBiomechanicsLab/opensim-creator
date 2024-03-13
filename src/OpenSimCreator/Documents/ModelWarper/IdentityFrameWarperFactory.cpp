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

std::vector<ValidationCheckResult> osc::mow::IdentityFrameWarperFactory::implValidate() const
{
    return {};
}

ValidationCheckState osc::mow::IdentityFrameWarperFactory::implState() const
{
    return ValidationCheckState::Ok;
}

std::unique_ptr<IFrameWarper> osc::mow::IdentityFrameWarperFactory::implTryCreateFrameWarper(ModelWarpDocument const&) const
{
    class IdentityFrameWarper final : public IFrameWarper {
    private:
        SimTK::Transform implWarp(SimTK::Transform const& transform) const
        {
            return transform;  // identity
        }
    };
    return std::make_unique<IdentityFrameWarper>();
}
