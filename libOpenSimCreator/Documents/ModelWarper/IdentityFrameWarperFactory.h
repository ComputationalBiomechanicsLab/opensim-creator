#pragma once

#include <libOpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <libOpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <libOpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <libOpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <libOpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <memory>
#include <vector>

namespace osc::mow { class WarpableModel; }

namespace osc::mow
{
    // an `IFrameWarperFactory` that produces an `IFrameWarper` that does nothing
    //
    // (useful for skipping a frame warp, warns the user that it's happening)
    class IdentityFrameWarperFactory final : public IFrameWarperFactory {
    private:
        std::unique_ptr<IFrameWarperFactory> implClone() const override;
        std::vector<WarpDetail> implWarpDetails() const override;
        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const override;
        std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(const WarpableModel&) const override;
    };
}
