#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IDetailListable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>

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
    };
}
