#include "Body.h"

#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIStrings.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/TransformFunctions.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Utils/UID.h>

#include <iostream>
#include <string>
#include <string_view>

using osc::mi::MIClass;

osc::mi::Body::Body(
    UID id,
    const std::string& name,
    const Transform& xform) :

    m_ID{id},
    m_Name{SanitizeToOpenSimComponentName(name)},
    m_Xform{xform}
{
}

void osc::mi::Body::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}

MIClass osc::mi::Body::CreateClass()
{
    return
    {
        MIStrings::c_BodyLabel,
        MIStrings::c_BodyLabelPluralized,
        MIStrings::c_BodyLabelOptionallyPluralized,
        OSC_ICON_CIRCLE,
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
