#pragma once

namespace opyn
{
    // A single, potentially user-provided, scaling parameter to the model warper.
    //
    // It is the responsibility of the engine/UI to gather/provide this to the
    // scaling engine at scale-time.
    using ScalingParameterValue = double;
}
