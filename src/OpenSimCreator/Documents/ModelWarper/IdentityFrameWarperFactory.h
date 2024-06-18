#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <memory>
#include <vector>

namespace osc::mow { class ModelWarpDocument; }

namespace osc::mow
{
    // an `IFrameWarperFactory` that produces an `IFrameWarper` that does nothing
    //
    // (useful for skipping a frame warp, warns the user that it's happening)
    class IdentityFrameWarperFactory final : public IFrameWarperFactory {
    private:
        std::unique_ptr<IFrameWarperFactory> implClone() const override;
        std::vector<WarpDetail> implWarpDetails() const override;
        std::vector<ValidationCheckResult> implValidate() const override;
        std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(const ModelWarpDocument&) const override;
    };
}
