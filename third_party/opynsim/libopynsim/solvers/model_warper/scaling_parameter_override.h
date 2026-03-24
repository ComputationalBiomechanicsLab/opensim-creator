#pragma once

#include <libopynsim/solvers/model_warper/scaling_parameter_value.h>

#include <OpenSim/Common/Object.h>

#include <string>

namespace opyn
{
    // A scaling parameter value override, which is usually provided by the top-level
    // document to override the default that a `ScalingStep` would typically declare
    // in a `ScalingParameterDeclaration`.
    class ScalingParameterOverride final : public OpenSim::Object {
        OpenSim_DECLARE_CONCRETE_OBJECT(ScalingParameterOverride, OpenSim::Object)
    public:
        OpenSim_DECLARE_PROPERTY(parameter_name, std::string, "The name of the scaling parameter that should have its default overridden.");
        OpenSim_DECLARE_PROPERTY(parameter_value, ScalingParameterValue, "The value to override the scaling parameter with. Note: it must have the correct datatype for the given scaling parameter.");

        explicit ScalingParameterOverride()
        {
            constructProperty_parameter_name("unknown");
            constructProperty_parameter_value(1.0);
        }

        explicit ScalingParameterOverride(const std::string& name, ScalingParameterValue value)
        {
            constructProperty_parameter_name(name);
            constructProperty_parameter_value(value);
        }
    };
}
