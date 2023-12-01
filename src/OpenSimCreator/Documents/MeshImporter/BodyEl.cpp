#include "BodyEl.hpp"

#include <OpenSimCreator/Documents/MeshImporter/ModelGraphStrings.hpp>
#include <OpenSimCreator/Documents/MeshImporter/SceneElClass.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>
#include <string>
#include <string_view>

osc::BodyEl::BodyEl(
    UID id,
    std::string const& name,
    Transform const& xform) :

    m_ID{id},
    m_Name{SanitizeToOpenSimComponentName(name)},
    m_Xform{xform}
{
}

void osc::BodyEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}

osc::SceneElClass osc::BodyEl::CreateClass()
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

std::ostream& osc::BodyEl::implWriteToStream(std::ostream& o) const
{
    return o << "BodyEl(ID = " << m_ID
        << ", Name = " << m_Name
        << ", m_Transform = " << m_Xform
        << ", Mass = " << Mass
        << ')';
}
