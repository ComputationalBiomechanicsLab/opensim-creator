#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>

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
    public:
        virtual ~IValidateable() noexcept = default;
        std::vector<ValidationCheck> validate() const { return implValidate(); }
        ValidationCheck::State state() const { return implState(); }
    private:
        virtual std::vector<ValidationCheck> implValidate() const  { return {}; }
        virtual ValidationCheck::State implState() const;  // by default, gets worst entry in `validate`
    };
}
