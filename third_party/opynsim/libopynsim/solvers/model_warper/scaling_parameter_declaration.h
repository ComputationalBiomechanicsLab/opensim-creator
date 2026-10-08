#pragma once

#include <libopynsim/solvers/model_warper/scaling_parameter_value.h>

#include <string>
#include <utility>

namespace opyn
{
    // A declaration of a scaling parameter for the model warper.
    //
    // `ScalingStep`s can declare that they may/must use a named `ScalingParameterValue`s
    // at runtime. This class is how they express that requirement. It's the scaling
    // engine/UI's responsibility to provide `ScalingParameters` that satisfy all
    // `ScalingStep`'s declarations.
    class ScalingParameterDeclaration final {
    public:
        explicit ScalingParameterDeclaration(std::string name, ScalingParameterValue defaultValue) :
            name_{std::move(name)},
            default_value_{defaultValue}
        {}

        const std::string& name() const { return name_; }
        const ScalingParameterValue& default_value() const { return default_value_; }
    private:
        std::string name_;
        ScalingParameterValue default_value_;
    };
}
