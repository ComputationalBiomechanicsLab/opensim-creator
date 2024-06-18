#pragma once

#include <Simbody.h>

namespace osc::mow
{
    // an interface to an object that can warp a frame (i.e. a rotation + translation transform)
    class IFrameWarper {
    protected:
        IFrameWarper() = default;
        IFrameWarper(const IFrameWarper&) = default;
        IFrameWarper(IFrameWarper&&) noexcept = default;
        IFrameWarper& operator=(const IFrameWarper&) = default;
        IFrameWarper& operator=(IFrameWarper&&) noexcept = default;
    public:
        virtual ~IFrameWarper() noexcept = default;

        SimTK::Transform warp(const SimTK::Transform& transform) const
        {
            return implWarp(transform);
        }

    private:
        virtual SimTK::Transform implWarp(const SimTK::Transform&) const = 0;
    };
}
