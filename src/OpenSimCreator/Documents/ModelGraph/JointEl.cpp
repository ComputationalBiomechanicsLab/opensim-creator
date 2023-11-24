#include "JointEl.hpp"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/ModelGraph/CrossrefDirection.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

osc::JointEl::JointEl(
    UID id,
    size_t jointTypeIdx,
    std::string const& userAssignedName,  // can be empty
    UID parent,
    UID child,
    Transform const& xform) :

    m_ID{id},
    m_JointTypeIndex{jointTypeIdx},
    m_UserAssignedName{osc::SanitizeToOpenSimComponentName(userAssignedName)},
    m_Parent{parent},
    m_Child{child},
    m_Xform{xform}
{
}

osc::SceneElClass osc::JointEl::CreateClass()
{
    return
    {
        ModelGraphStrings::c_JointLabel,
        ModelGraphStrings::c_JointLabelPluralized,
        ModelGraphStrings::c_JointLabelOptionallyPluralized,
        ICON_FA_LINK,
        ModelGraphStrings::c_JointDescription,
    };
}

std::vector<osc::CrossrefDescriptor> osc::JointEl::implGetCrossReferences() const
{
    return
    {
        {m_Parent, ModelGraphStrings::c_JointParentCrossrefName, CrossrefDirection::ToParent},
        {m_Child,  ModelGraphStrings::c_JointChildCrossrefName,  CrossrefDirection::ToChild },
    };
}

osc::CStringView osc::JointEl::getSpecificTypeName() const
{
    return osc::At(osc::GetComponentRegistry<OpenSim::Joint>(), m_JointTypeIndex).name();
}

std::ostream& osc::JointEl::implWriteToStream(std::ostream& o) const
{
    return o << "JointEl(ID = " << m_ID
        << ", JointTypeIndex = " << m_JointTypeIndex
        << ", UserAssignedName = " << m_UserAssignedName
        << ", Parent = " << m_Parent
        << ", Child = " << m_Child
        << ", m_Transform = " << m_Xform
        << ')';
}

void osc::JointEl::implSetLabel(std::string_view sv)
{
    m_UserAssignedName = osc::SanitizeToOpenSimComponentName(sv);
}
