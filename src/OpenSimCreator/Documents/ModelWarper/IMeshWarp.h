#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Detail.h>
#include <OpenSimCreator/Documents/ModelWarper/IDetailListable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidationCheckable.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>

#include <oscar/Utils/ICloneable.h>

#include <functional>
#include <memory>

namespace osc::mow
{
    class IMeshWarp :
        public ICloneable<IMeshWarp>,
        public IDetailListable,
        public IValidationCheckable {
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
