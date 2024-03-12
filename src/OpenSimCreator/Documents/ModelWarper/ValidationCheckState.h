#pragma once

namespace osc::mow
{
    // possible states of a runtime validation check for an element in the model warper
    enum class ValidationCheckState {
        Ok,
        Warning,
        Error,
        NUM_OPTIONS
    };
}
