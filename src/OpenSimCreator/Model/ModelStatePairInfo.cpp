#include "ModelStatePairInfo.hpp"

#include "OpenSimCreator/Model/VirtualConstModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

osc::ModelStatePairInfo::ModelStatePairInfo() = default;
osc::ModelStatePairInfo::ModelStatePairInfo(VirtualConstModelStatePair const& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{
}

bool osc::operator==(ModelStatePairInfo const& a, ModelStatePairInfo const& b) noexcept
{
    return 
        a.m_ModelVersion == b.m_ModelVersion &&
        a.m_StateVersion == b.m_StateVersion &&
        a.m_Selection == b.m_Selection &&
        a.m_Hover == b.m_Hover &&
        a.m_FixupScaleFactor == b.m_FixupScaleFactor;
}

bool osc::operator!=(ModelStatePairInfo const& a, ModelStatePairInfo const& b) noexcept
{
    return !(a == b);
}