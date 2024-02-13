#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IDetailListable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidationCheckable.h>

#include <oscar/Utils/ICloneable.h>

namespace osc::mow
{
    class IFrameWarp :
        public ICloneable<IFrameWarp>,
        public IDetailListable,
        public IValidationCheckable {
    protected:
        IFrameWarp() = default;
        IFrameWarp(IFrameWarp const&) = default;
        IFrameWarp& operator=(IFrameWarp const&) = default;
        IFrameWarp& operator=(IFrameWarp&&) noexcept = default;
    public:
        virtual ~IFrameWarp() noexcept = default;
    };
}
