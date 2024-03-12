#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/IFrameWarperFactory.h>

#include <Simbody.h>

#include <memory>

namespace osc::mow
{
    // an `IFrameWarperFactory` that produces an `IFrameWarper` that does nothing
    //
    // (useful for skipping warping something)
    class IdentityFrameWarperFactory final : public IFrameWarperFactory {
    private:
        std::unique_ptr<IFrameWarperFactory> implClone() const override { return std::make_unique<IdentityFrameWarperFactory>(*this); }
        std::vector<WarpDetail> implWarpDetails() const override { return {}; }
        std::vector<ValidationCheckResult> implValidate() const override { return {}; }
        ValidationCheckState implState() const override { return ValidationCheckState::Ok; }
        std::unique_ptr<IFrameWarper> implTryCreateFrameWarper(ModelWarpDocument const&) const override
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
    };
}
