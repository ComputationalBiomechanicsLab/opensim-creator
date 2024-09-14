#include "Joint.h"

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.h>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TransformFunctions.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

using osc::mi::CrossrefDescriptor;
using osc::mi::MIClass;

osc::mi::Joint::Joint(
    UID id,
    std::string jointTypeName,
    const std::string& userAssignedName,  // can be empty
    UID parent,
    UID child,
    const Transform& xform) :

    m_ID{id},
    m_JointTypeName{std::move(jointTypeName)},
    m_UserAssignedName{SanitizeToOpenSimComponentName(userAssignedName)},
    m_Parent{parent},
    m_Child{child},
    m_Xform{xform}
{}

MIClass osc::mi::Joint::CreateClass()
{
    return
    {
        MIStrings::c_JointLabel,
        MIStrings::c_JointLabelPluralized,
        MIStrings::c_JointLabelOptionallyPluralized,
        OSC_ICON_LINK,
        MIStrings::c_JointDescription,
    };
}

std::vector<CrossrefDescriptor> osc::mi::Joint::implGetCrossReferences() const
{
    return
    {
        {m_Parent, MIStrings::c_JointParentCrossrefName, CrossrefDirection::ToParent},
        {m_Child,  MIStrings::c_JointChildCrossrefName,  CrossrefDirection::ToChild },
    };
}

std::ostream& osc::mi::Joint::implWriteToStream(std::ostream& o) const
{
    return o << "Joint(ID = " << m_ID
        << ", JointTypeName = " << m_JointTypeName
        << ", UserAssignedName = " << m_UserAssignedName
        << ", Parent = " << m_Parent
        << ", Child = " << m_Child
        << ", Transform = " << m_Xform
        << ')';
}

void osc::mi::Joint::implSetLabel(std::string_view sv)
{
    m_UserAssignedName = SanitizeToOpenSimComponentName(sv);
}
