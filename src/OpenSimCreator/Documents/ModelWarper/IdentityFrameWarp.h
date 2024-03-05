#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IFrameWarp.h>

#include <memory>

namespace osc::mow
{
    class IdentityFrameWarp final : public IFrameWarp {
    private:
        std::unique_ptr<IFrameWarp> implClone() const override { return std::make_unique<IdentityFrameWarp>(*this); }
        std::vector<WarpDetail> implWarpDetails() const override { return {}; }
        std::vector<ValidationCheck> implValidate() const override { return {}; }
        ValidationState implState() const override { return ValidationState::Ok; }
    };
}
