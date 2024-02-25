#include "Joint.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.h>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TransformFunctions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

using osc::mi::CrossrefDescriptor;
using osc::mi::MIClass;
using osc::CStringView;

osc::mi::Joint::Joint(
    UID id,
    size_t jointTypeIdx,
    std::string const& userAssignedName,  // can be empty
    UID parent,
    UID child,
    Transform const& xform) :

    m_ID{id},
    m_JointTypeIndex{jointTypeIdx},
    m_UserAssignedName{SanitizeToOpenSimComponentName(userAssignedName)},
    m_Parent{parent},
    m_Child{child},
    m_Xform{xform}
{
}

MIClass osc::mi::Joint::CreateClass()
{
    return
    {
        MIStrings::c_JointLabel,
        MIStrings::c_JointLabelPluralized,
        MIStrings::c_JointLabelOptionallyPluralized,
        ICON_FA_LINK,
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

CStringView osc::mi::Joint::getSpecificTypeName() const
{
    return At(GetComponentRegistry<OpenSim::Joint>(), m_JointTypeIndex).name();
}

std::ostream& osc::mi::Joint::implWriteToStream(std::ostream& o) const
{
    return o << "Joint(ID = " << m_ID
        << ", JointTypeIndex = " << m_JointTypeIndex
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
