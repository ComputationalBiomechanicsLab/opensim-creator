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
    // an `IFrameWarperFactory` that produces an `IFrameWarper` that does nothing, because
    // `StationDefinedFrame`s are automatically warped once the model's `Station`s are warped
    //
    // in contrast to a `IdentityFrameWarperFactory`, does not warn the user - warping this
    // type of frame is well-defined
    class StationDefinedFrameWarperFactory final : public IFrameWarperFactory {
    private:
        std::unique_ptr<IFrameWarperFactory> implClone() const override;
        std::vector<WarpDetail> implWarpDetails() const override;
        std::vector<ValidationCheckResult> implValidate() const override;
        std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(ModelWarpDocument const&) const override;
    };
}
