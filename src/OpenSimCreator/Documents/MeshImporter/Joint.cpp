#include "Joint.hpp"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

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
        ModelGraphStrings::c_JointLabel,
        ModelGraphStrings::c_JointLabelPluralized,
        ModelGraphStrings::c_JointLabelOptionallyPluralized,
        ICON_FA_LINK,
        ModelGraphStrings::c_JointDescription,
    };
}

std::vector<CrossrefDescriptor> osc::mi::Joint::implGetCrossReferences() const
{
    return
    {
        {m_Parent, ModelGraphStrings::c_JointParentCrossrefName, CrossrefDirection::ToParent},
        {m_Child,  ModelGraphStrings::c_JointChildCrossrefName,  CrossrefDirection::ToChild },
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
        << ", m_Transform = " << m_Xform
        << ')';
}

void osc::mi::Joint::implSetLabel(std::string_view sv)
{
    m_UserAssignedName = SanitizeToOpenSimComponentName(sv);
}
