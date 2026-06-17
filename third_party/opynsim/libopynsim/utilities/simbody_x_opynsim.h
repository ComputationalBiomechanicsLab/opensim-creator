#pragma once

#include <libopynsim/model_state_stage.h>

#include <SimTKcommon/internal/Stage.h>

namespace opyn
{
    ModelStateStage to_opynsim_stage(SimTK::Stage);
    SimTK::Stage to_simbody_stage(ModelStateStage);
}