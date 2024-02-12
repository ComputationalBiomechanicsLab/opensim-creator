#pragma once

#include <oscar/Utils/ICloneable.h>

namespace osc::mow
{
    class IFrameWarp : public ICloneable<IFrameWarp> {
    protected:
        IFrameWarp() = default;
        IFrameWarp(IFrameWarp const&) = default;
        IFrameWarp& operator=(IFrameWarp const&) = default;
        IFrameWarp& operator=(IFrameWarp&&) noexcept = default;
    public:
        virtual ~IFrameWarp() noexcept = default;
    };
}
