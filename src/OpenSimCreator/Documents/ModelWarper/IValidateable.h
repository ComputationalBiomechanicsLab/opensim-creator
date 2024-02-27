#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>

#include <vector>

namespace osc::mow
{
    class IValidateable {
    protected:
        IValidateable() = default;
        IValidateable(IValidateable const&) = default;
        IValidateable(IValidateable&&) noexcept = default;
        IValidateable& operator=(IValidateable const&) = default;
        IValidateable& operator=(IValidateable&&) noexcept = default;

        friend bool operator==(IValidateable const&, IValidateable const&) = default;
    public:
        virtual ~IValidateable() noexcept = default;
        std::vector<ValidationCheck> validate() const { return implValidate(); }
        ValidationState state() const { return implState(); }
    private:
        virtual std::vector<ValidationCheck> implValidate() const  { return {}; }
        virtual ValidationState implState() const;  // by default, gets the "worst" entry returned by `validate`
    };
}
