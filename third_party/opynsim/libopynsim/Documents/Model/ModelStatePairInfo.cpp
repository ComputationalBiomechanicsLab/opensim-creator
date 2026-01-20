#include "ModelStatePairInfo.h"

#include <libopynsim/Documents/Model/ModelStatePair.h>
#include <libopynsim/Utils/OpenSimHelpers.h>

osc::ModelStatePairInfo::ModelStatePairInfo() = default;

osc::ModelStatePairInfo::ModelStatePairInfo(const ModelStatePair& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{}
