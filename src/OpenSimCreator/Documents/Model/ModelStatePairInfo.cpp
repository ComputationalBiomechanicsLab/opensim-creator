#include "ModelStatePairInfo.h"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

osc::ModelStatePairInfo::ModelStatePairInfo() = default;

osc::ModelStatePairInfo::ModelStatePairInfo(IConstModelStatePair const& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{}
