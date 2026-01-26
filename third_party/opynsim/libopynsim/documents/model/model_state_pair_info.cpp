#include "model_state_pair_info.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>

using namespace opyn;

osc::ModelStatePairInfo::ModelStatePairInfo() = default;

osc::ModelStatePairInfo::ModelStatePairInfo(const ModelStatePair& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{}
