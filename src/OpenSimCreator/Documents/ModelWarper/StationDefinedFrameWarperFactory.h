#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <memory>
#include <vector>

namespace osc::mow { class WarpableModel; }

namespace osc::mow
{
    // an `IFrameWarperFactory` that produces an `IFrameWarper` that does nothing
    //
    // - it does nothing because `StationDefinedFrame`s only require that the `IPointWarper`s
    //   warp all stations in the model (which, by default, they do)
    //
    // - in contrast to a `IdentityFrameWarperFactory`, this frame warper does not warn the user
    //   that there's a problem, because `StationDefinedFrame` warps are well-defined
    class StationDefinedFrameWarperFactory final : public IFrameWarperFactory {
    private:
        std::unique_ptr<IFrameWarperFactory> implClone() const override;
        std::vector<WarpDetail> implWarpDetails() const override;
        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const override;
        std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(const WarpableModel&) const override;
    };
}
