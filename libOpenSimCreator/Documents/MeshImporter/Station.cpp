#include "Station.h"

#include <libopensimcreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <libopensimcreator/Documents/MeshImporter/CrossrefDirection.h>
#include <libopensimcreator/Documents/MeshImporter/MIStrings.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Maths/VecFunctions.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Utils/UID.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using osc::mi::CrossrefDescriptor;
using osc::mi::MIClass;

osc::mi::StationEl::StationEl(
    UID id,
    UID attachment,
    const Vec3& position,
    const std::string& name) :

    m_ID{id},
    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

osc::mi::StationEl::StationEl(
    UID attachment,
    const Vec3& position,
    const std::string& name) :

    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

MIClass osc::mi::StationEl::CreateClass()
{
    return
    {
        MIStrings::c_StationLabel,
        MIStrings::c_StationLabelPluralized,
        MIStrings::c_StationLabelOptionallyPluralized,
        OSC_ICON_MAP_PIN,
        MIStrings::c_StationDescription,
    };
}

std::vector<CrossrefDescriptor> osc::mi::StationEl::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, MIStrings::c_StationParentCrossrefName, CrossrefDirection::ToParent},
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
