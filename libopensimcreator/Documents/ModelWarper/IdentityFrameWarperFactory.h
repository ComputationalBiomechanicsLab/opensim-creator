#pragma once

#include <libopensimcreator/Documents/ModelWarper/IFrameWarper.h>
#include <libopensimcreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <libopensimcreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <libopensimcreator/Documents/ModelWarper/ValidationCheckState.h>
#include <libopensimcreator/Documents/ModelWarper/WarpDetail.h>

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
