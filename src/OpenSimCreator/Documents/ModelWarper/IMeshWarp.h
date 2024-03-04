#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IDetailListable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>

#include <oscar/Maths/Vec3.h>

#include <span>

namespace osc::mow
{
    class IMeshWarp :
        public ICloneable<IMeshWarp>,
        public IDetailListable,
        public IValidateable {
    protected:
        IMeshWarp() = default;
        IMeshWarp(IMeshWarp const&) = default;
        IMeshWarp(IMeshWarp&&) noexcept = default;
        IMeshWarp& operator=(IMeshWarp const&) = default;
        IMeshWarp& operator=(IMeshWarp&&) noexcept = default;
    public:
        virtual ~IMeshWarp() = default;

        void warpInPlace(std::span<Vec3> points) const { return implWarpInPlace(points); }
    private:
        virtual void implWarpInPlace(std::span<Vec3>) const = 0;
    };
}
