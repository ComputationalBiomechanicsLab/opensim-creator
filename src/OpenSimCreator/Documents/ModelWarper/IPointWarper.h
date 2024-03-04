#pragma once

#include <oscar/Maths/Vec3.h>

#include <span>

namespace osc
{
    class IPointWarper {
    protected:
        IPointWarper() = default;
        IPointWarper(IPointWarper const&) = default;
        IPointWarper(IPointWarper&&) noexcept = default;
        IPointWarper& operator=(IPointWarper const&) = default;
        IPointWarper& operator=(IPointWarper&&) noexcept = default;
    public:
        virtual ~IPointWarper() noexcept = default;

        void warpInPlace(std::span<Vec3> points) const { implWarpInPlace(points); }
    private:
        virtual void implWarpInPlace(std::span<Vec3>) const = 0;
    };
}
