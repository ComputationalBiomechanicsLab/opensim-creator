#pragma once

namespace opyn
{
    enum class ModelStateStage {
        time = 0,
        position,
        velocity,
        dynamics,
        acceleration,
        report,

        NUM_OPTIONS
    };
}
