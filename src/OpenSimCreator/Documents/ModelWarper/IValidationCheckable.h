#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>

#include <functional>

namespace osc::mow
{
    class IValidationCheckable {
    protected:
        IValidationCheckable() = default;
        IValidationCheckable(IValidationCheckable const&) = default;
        IValidationCheckable(IValidationCheckable&&) noexcept = default;
        IValidationCheckable& operator=(IValidationCheckable const&) = default;
        IValidationCheckable& operator=(IValidationCheckable&&) noexcept = default;
    public:
        virtual ~IValidationCheckable() noexcept = default;

        void forEachCheck(std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const { implForEachCheck(callback); }
        ValidationCheck::State state() const { return implState(); }
    private:
        virtual void implForEachCheck(std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const = 0;
        virtual ValidationCheck::State implState() const = 0;
    };
}
