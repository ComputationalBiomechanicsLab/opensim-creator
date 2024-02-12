#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Detail.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>

#include <oscar/Interfaces/ICloneable.h>

#include <functional>
#include <memory>

namespace osc::mow
{
    class IMeshWarp : public ICloneable<IMeshWarp> {
    protected:
        IMeshWarp() = default;
        IMeshWarp(IMeshWarp const&) = default;
        IMeshWarp(IMeshWarp&&) noexcept = default;
        IMeshWarp& operator=(IMeshWarp const&) = default;
        IMeshWarp& operator=(IMeshWarp&&) noexcept = default;
    public:
        virtual ~IMeshWarp() = default;

        void forEachDetail(std::function<void(Detail)> const& callback) const { implForEachDetail(callback); }
        void forEachCheck(std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const { implForEachCheck(callback); }
        ValidationCheck::State state() const { return implState(); }

    private:
        virtual void implForEachDetail(std::function<void(Detail)> const&) const = 0;
        virtual void implForEachCheck(std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const = 0;
        virtual ValidationCheck::State implState() const = 0;
    };
}
