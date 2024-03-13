#pragma once

#include <Simbody.h>

namespace osc::mow
{
    // an interface to an object that can warp a frame (i.e. a rotation + translation transform)
    class IFrameWarper {
    protected:
        IFrameWarper() = default;
        IFrameWarper(IFrameWarper const&) = default;
        IFrameWarper(IFrameWarper&&) noexcept = default;
        IFrameWarper& operator=(IFrameWarper const&) = default;
        IFrameWarper& operator=(IFrameWarper&&) noexcept = default;
    public:
        virtual ~IFrameWarper() noexcept = default;

        SimTK::Transform warp(SimTK::Transform const& transform) const
        {
            return implWarp(transform);
        }

    private:
        virtual SimTK::Transform implWarp(SimTK::Transform const&) const = 0;
    };
}
