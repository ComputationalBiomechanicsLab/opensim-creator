#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <vector>

namespace osc::mow
{
    // an interface to an object that can be runtime-validated
    class IValidateable {
    protected:
        IValidateable() = default;
        IValidateable(const IValidateable&) = default;
        IValidateable(IValidateable&&) noexcept = default;
        IValidateable& operator=(const IValidateable&) = default;
        IValidateable& operator=(IValidateable&&) noexcept = default;

        friend bool operator==(const IValidateable&, const IValidateable&) = default;
    public:
        virtual ~IValidateable() noexcept = default;
        std::vector<ValidationCheckResult> validate() const { return implValidate(); }
        ValidationCheckState state() const { return implState(); }
    private:
        virtual std::vector<ValidationCheckResult> implValidate() const  { return {}; }
        virtual ValidationCheckState implState() const;  // by default, gets the least-valid entry returned by `validate()`
    };
}
