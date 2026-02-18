#include "mi_body.h"

#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/uid.h>

#include <ostream>
#include <string>
#include <string_view>

using osc::MiClass;

osc::MiBody::MiBody(
    UID id,
    const std::string& name,
    const Transform& xform) :

    m_ID{id},
    m_Name{opyn::SanitizeToOpenSimComponentName(name)},
    m_Xform{xform}
{
}

void osc::MiBody::implSetLabel(std::string_view sv)
{
    m_Name = opyn::SanitizeToOpenSimComponentName(sv);
}

MiClass osc::MiBody::CreateClass()
{
    return
    {
        MiStrings::c_BodyLabel,
        MiStrings::c_BodyLabelPluralized,
        MiStrings::c_BodyLabelOptionallyPluralized,
        MSMICONS_CIRCLE,
        MiStrings::c_BodyDescription,
    };
}

std::ostream& osc::MiBody::implWriteToStream(std::ostream& o) const
{
    return o << "MiBody(ID = " << m_ID
        << ", Name = " << m_Name
        << ", m_Transform = " << m_Xform
        << ", Mass = " << Mass
        << ')';
}
