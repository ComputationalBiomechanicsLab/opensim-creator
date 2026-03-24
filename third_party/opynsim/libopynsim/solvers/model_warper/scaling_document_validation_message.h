#pragma once

#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>

#include <OpenSim/Common/ComponentPath.h>

namespace opyn
{
    // A top-level message produced by validating an entire scaling document (not just `ScalingStep`s,
    // but any top-level validation concerns also). In MVC parlance, this is the M - so it shouldn't
    // directly use or refer to the UI.
    struct ScalingDocumentValidationMessage {
        OpenSim::ComponentPath sourceScalingStepAbsPath;
        ScalingStepValidationMessage payload;
    };
}
