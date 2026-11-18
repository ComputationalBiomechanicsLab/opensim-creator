#include "model_state_stage.h"

#include <string_view>

std::string_view opyn::to_string_view(ModelStateStage model_state_stage)
{
    switch (model_state_stage) {
    case ModelStateStage::topology: return "ModelStateStage.TOPOLOGY";
    case ModelStateStage::model: return "ModelStateStage.MODEL";
    case ModelStateStage::instance: return "ModelStateStage.INSTANCE";
    case ModelStateStage::time: return "ModelStateStage.TIME";
    case ModelStateStage::position: return "ModelStateStage.POSITION";
    case ModelStateStage::velocity: return "ModelStateStage.VELOCITY";
    case ModelStateStage::dynamics: return "ModelStateStage.DYNAMICS";
    case ModelStateStage::acceleration: return "ModelStateStage.ACCELERATION";
    case ModelStateStage::report: return "ModelStateStage.REPORT";
    default: return "ModelStateStage.INVALID";
    }
}
