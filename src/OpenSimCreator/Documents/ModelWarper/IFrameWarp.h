#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IDetailListable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>

namespace osc::mow
{
    class IFrameWarp :
        public ICloneable<IFrameWarp>,
        public IDetailListable,
        public IValidateable {
    protected:
        IFrameWarp() = default;
        IFrameWarp(IFrameWarp const&) = default;
        IFrameWarp& operator=(IFrameWarp const&) = default;
        IFrameWarp& operator=(IFrameWarp&&) noexcept = default;
    public:
        virtual ~IFrameWarp() noexcept = default;
    };
}
