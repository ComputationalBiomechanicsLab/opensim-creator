#include "Body.h"

#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIStrings.h>
#include <libopensimcreator/Platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utils/uid.h>

#include <ostream>
#include <string>
#include <string_view>

using osc::mi::MIClass;

osc::mi::Body::Body(
    UID id,
    const std::string& name,
    const Transform& xform) :

    m_ID{id},
    m_Name{opyn::SanitizeToOpenSimComponentName(name)},
    m_Xform{xform}
{
}

void osc::mi::Body::implSetLabel(std::string_view sv)
{
    m_Name = opyn::SanitizeToOpenSimComponentName(sv);
}

MIClass osc::mi::Body::CreateClass()
{
    return
    {
        MIStrings::c_BodyLabel,
        MIStrings::c_BodyLabelPluralized,
        MIStrings::c_BodyLabelOptionallyPluralized,
        MSMICONS_CIRCLE,
        MIStrings::c_BodyDescription,
    };
}

std::ostream& osc::mi::Body::implWriteToStream(std::ostream& o) const
{
    return o << "Body(ID = " << m_ID
        << ", Name = " << m_Name
        << ", m_Transform = " << m_Xform
        << ", Mass = " << Mass
        << ')';
}
