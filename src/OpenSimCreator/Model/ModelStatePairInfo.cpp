#include "ModelStatePairInfo.hpp"

#include <OpenSimCreator/Model/VirtualConstModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

osc::ModelStatePairInfo::ModelStatePairInfo() = default;
osc::ModelStatePairInfo::ModelStatePairInfo(VirtualConstModelStatePair const& msp) :
    m_ModelVersion{msp.getModelVersion()},
    m_StateVersion{msp.getStateVersion()},
    m_Selection{GetAbsolutePathOrEmpty(msp.getSelected())},
    m_Hover{GetAbsolutePathOrEmpty(msp.getHovered())},
    m_FixupScaleFactor{msp.getFixupScaleFactor()}
{
}

bool osc::operator==(ModelStatePairInfo const& lhs, ModelStatePairInfo const& rhs) noexcept
{
    return
        lhs.m_ModelVersion == rhs.m_ModelVersion &&
        lhs.m_StateVersion == rhs.m_StateVersion &&
        lhs.m_Selection == rhs.m_Selection &&
        lhs.m_Hover == rhs.m_Hover &&
        lhs.m_FixupScaleFactor == rhs.m_FixupScaleFactor;
}

bool osc::operator!=(ModelStatePairInfo const& lhs, ModelStatePairInfo const& rhs) noexcept
{
    return !(lhs == rhs);
}
