#include "Body.hpp"

#include <OpenSimCreator/Documents/MeshImporter/MIClass.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>
#include <string>
#include <string_view>

using osc::mi::MIClass;

osc::mi::Body::Body(
    UID id,
    std::string const& name,
    Transform const& xform) :

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
        ModelGraphStrings::c_BodyLabel,
        ModelGraphStrings::c_BodyLabelPluralized,
        ModelGraphStrings::c_BodyLabelOptionallyPluralized,
        ICON_FA_CIRCLE,
        ModelGraphStrings::c_BodyDescription,
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
