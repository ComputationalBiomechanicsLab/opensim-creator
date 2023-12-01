#include "Station.hpp"

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using osc::mi::CrossrefDescriptor;
using osc::mi::MIClass;

osc::mi::StationEl::StationEl(
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

osc::mi::StationEl::StationEl(
    UID attachment,
    Vec3 const& position,
    std::string const& name) :

    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

MIClass osc::mi::StationEl::CreateClass()
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

std::vector<CrossrefDescriptor> osc::mi::StationEl::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, ModelGraphStrings::c_StationParentCrossrefName, CrossrefDirection::ToParent},
    };
}

std::ostream& osc::mi::StationEl::implWriteToStream(std::ostream& o) const
{
    return o << "StationEl("
        << "ID = " << m_ID
        << ", Attachment = " << m_Attachment
        << ", Position = " << m_Position
        << ", Name = " << m_Name
        << ')';
}

void osc::mi::StationEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
