#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <vector>

namespace osc::mow { class WarpableModel; }

namespace osc::mow
{
    // an interface to an object that can be runtime-validated against the
    // root document
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
        std::vector<ValidationCheckResult> validate(const WarpableModel& root) const { return implValidate(root); }
        ValidationCheckState state(const WarpableModel& root) const { return implState(root); }

    private:
        // by default, returns an empty list of `ValidationCheckResult`s
        virtual std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const;

        // by default, gets the least-valid entry returned by `validate()`
        virtual ValidationCheckState implState(const WarpableModel&) const;
    };
}
