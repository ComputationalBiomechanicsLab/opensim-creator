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
            m_Name{std::move(name)},
            m_DefaultValue{defaultValue}
        {}

        const std::string& name() const { return m_Name; }
        const ScalingParameterValue& default_value() const { return m_DefaultValue; }
    private:
        std::string m_Name;
        ScalingParameterValue m_DefaultValue;
    };
}
