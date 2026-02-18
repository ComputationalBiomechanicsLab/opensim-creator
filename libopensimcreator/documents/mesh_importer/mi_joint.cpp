#include "mi_joint.h"

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

using osc::MiCrossrefDescriptor;
using osc::MiClass;

osc::MiJoint::MiJoint(
    UID id,
    std::string jointTypeName,
    const std::string& userAssignedName,  // can be empty
    UID parent,
    UID child,
    const Transform& xform) :

    m_ID{id},
    m_JointTypeName{std::move(jointTypeName)},
    m_UserAssignedName{opyn::SanitizeToOpenSimComponentName(userAssignedName)},
    m_Parent{parent},
    m_Child{child},
    m_Xform{xform}
{}

MiClass osc::MiJoint::CreateClass()
{
    return
    {
        MiStrings::c_JointLabel,
        MiStrings::c_JointLabelPluralized,
        MiStrings::c_JointLabelOptionallyPluralized,
        MSMICONS_LINK,
        MiStrings::c_JointDescription,
    };
}

std::vector<MiCrossrefDescriptor> osc::MiJoint::implGetCrossReferences() const
{
    return
    {
        {m_Parent, MiStrings::c_JointParentCrossrefName, MiCrossrefDirection::ToParent},
        {m_Child,  MiStrings::c_JointChildCrossrefName,  MiCrossrefDirection::ToChild },
    };
}

std::ostream& osc::MiJoint::implWriteToStream(std::ostream& o) const
{
    return o << "MiJoint(ID = " << m_ID
        << ", JointTypeName = " << m_JointTypeName
        << ", UserAssignedName = " << m_UserAssignedName
        << ", Parent = " << m_Parent
        << ", Child = " << m_Child
        << ", Transform = " << m_Xform
        << ')';
}

void osc::MiJoint::implSetLabel(std::string_view sv)
{
    m_UserAssignedName = opyn::SanitizeToOpenSimComponentName(sv);
}
