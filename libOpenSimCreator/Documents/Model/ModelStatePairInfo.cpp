#include "ModelStatePairInfo.h"

#include <libOpenSimCreator/Documents/Model/IModelStatePair.h>
#include <libOpenSimCreator/Utils/OpenSimHelpers.h>

osc::ModelStatePairInfo::ModelStatePairInfo() = default;

osc::ModelStatePairInfo::ModelStatePairInfo(const IModelStatePair& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{}
