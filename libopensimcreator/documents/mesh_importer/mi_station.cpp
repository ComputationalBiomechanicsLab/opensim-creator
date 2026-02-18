#include "mi_station.h"

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/maths/vector_functions.h>
#include <liboscar/utilities/uid.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using osc::MiCrossrefDescriptor;
using osc::MiClass;

osc::MiStation::MiStation(
    UID id,
    UID attachment,
    const Vector3& position,
    const std::string& name) :

    m_ID{id},
    m_Attachment{attachment},
    m_Position{position},
    m_Name{opyn::SanitizeToOpenSimComponentName(name)}
{
}

osc::MiStation::MiStation(
    UID attachment,
    const Vector3& position,
    const std::string& name) :

    m_Attachment{attachment},
    m_Position{position},
    m_Name{opyn::SanitizeToOpenSimComponentName(name)}
{
}

MiClass osc::MiStation::CreateClass()
{
    return
    {
        MiStrings::c_StationLabel,
        MiStrings::c_StationLabelPluralized,
        MiStrings::c_StationLabelOptionallyPluralized,
        MSMICONS_MAP_PIN,
        MiStrings::c_StationDescription,
    };
}

std::vector<MiCrossrefDescriptor> osc::MiStation::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, MiStrings::c_StationParentCrossrefName, MiCrossrefDirection::ToParent},
    };
}

std::ostream& osc::MiStation::implWriteToStream(std::ostream& o) const
{
    return o << "MiStation("
        << "ID = " << m_ID
        << ", Attachment = " << m_Attachment
        << ", Position = " << m_Position
        << ", Name = " << m_Name
        << ')';
}

void osc::MiStation::implSetLabel(std::string_view sv)
{
    m_Name = opyn::SanitizeToOpenSimComponentName(sv);
}
