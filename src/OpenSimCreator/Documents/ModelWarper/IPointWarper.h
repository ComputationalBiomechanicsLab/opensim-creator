#pragma once

#include <oscar/Maths/Vec3.h>

#include <array>
#include <span>

namespace osc::mow
{
    // an interface to an object that can warp `Vec3`s
    class IPointWarper {
    protected:
        IPointWarper() = default;
        IPointWarper(const IPointWarper&) = default;
        IPointWarper(IPointWarper&&) noexcept = default;
        IPointWarper& operator=(const IPointWarper&) = default;
        IPointWarper& operator=(IPointWarper&&) noexcept = default;
    public:
        virtual ~IPointWarper() noexcept = default;

        Vec3 warp(Vec3 p) const
        {
            auto ap = std::to_array({p});
            implWarpInPlace(ap);
            return ap.front();
        }
        void warpInPlace(std::span<Vec3> points) const { implWarpInPlace(points); }
    private:
        virtual void implWarpInPlace(std::span<Vec3>) const = 0;
    };
}
