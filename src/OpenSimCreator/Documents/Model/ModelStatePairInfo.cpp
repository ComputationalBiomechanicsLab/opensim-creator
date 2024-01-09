#include "ModelStatePairInfo.hpp"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

osc::ModelStatePairInfo::ModelStatePairInfo() = default;
osc::ModelStatePairInfo::ModelStatePairInfo(IConstModelStatePair const& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{
}
