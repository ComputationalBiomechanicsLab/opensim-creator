#include "StationEl.hpp"

#include <OpenSimCreator/Documents/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/ModelGraph/CrossrefDirection.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

osc::StationEl::StationEl(
    UID id,
    UID attachment,
    Vec3 const& position,
    std::string const& name) :

    m_ID{id},
    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

osc::StationEl::StationEl(
    UID attachment,
    Vec3 const& position,
    std::string const& name) :

    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

osc::SceneElClass osc::StationEl::CreateClass()
{
    return
    {
        ModelGraphStrings::c_StationLabel,
        ModelGraphStrings::c_StationLabelPluralized,
        ModelGraphStrings::c_StationLabelOptionallyPluralized,
        ICON_FA_MAP_PIN,
        ModelGraphStrings::c_StationDescription,
    };
}

std::vector<osc::CrossrefDescriptor> osc::StationEl::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, ModelGraphStrings::c_StationParentCrossrefName, CrossrefDirection::ToParent},
    };
}

std::ostream& osc::StationEl::implWriteToStream(std::ostream& o) const
{
    return o << "StationEl("
        << "ID = " << m_ID
        << ", Attachment = " << m_Attachment
        << ", Position = " << m_Position
        << ", Name = " << m_Name
        << ')';
}

void osc::StationEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
