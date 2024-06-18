#include "Body.h"

#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TransformFunctions.h>
#include <oscar/Utils/UID.h>

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
        ICON_FA_CIRCLE,
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
