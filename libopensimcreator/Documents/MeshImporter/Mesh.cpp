#include "Mesh.h"

#include <libopensimcreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <libopensimcreator/Documents/MeshImporter/CrossrefDirection.h>
#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIStrings.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Utils/UID.h>

#include <filesystem>
#include <ostream>
#include <utility>
#include <vector>

using osc::mi::CrossrefDescriptor;
using osc::mi::MIClass;
using osc::AABB;

osc::mi::Mesh::Mesh(
    UID id,
    UID attachment,
    osc::Mesh meshData,
    std::filesystem::path path) :

    m_ID{id},
    m_Attachment{attachment},
    m_MeshData{std::move(meshData)},
    m_Path{std::move(path)},
    m_Name{SanitizeToOpenSimComponentName(m_Path.filename().replace_extension().string())}
{}

AABB osc::mi::Mesh::calcBounds() const
{
    return transform_aabb(m_Transform, m_MeshData.bounds());
}

MIClass osc::mi::Mesh::CreateClass()
{
    return
    {
        MIStrings::c_MeshLabel,
        MIStrings::c_MeshLabelPluralized,
        MIStrings::c_MeshLabelOptionallyPluralized,
        OSC_ICON_CUBE,
        MIStrings::c_MeshDescription,
    };
}

std::vector<CrossrefDescriptor> osc::mi::Mesh::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, MIStrings::c_MeshAttachmentCrossrefName, CrossrefDirection::ToParent},
    };
}

std::ostream& osc::mi::Mesh::implWriteToStream(std::ostream& o) const
{
    return o << "Mesh("
        << "ID = " << m_ID
        << ", Attachment = " << m_Attachment
        << ", m_Transform = " << m_Transform
        << ", MeshData = " << &m_MeshData
        << ", Path = " << m_Path
        << ", Name = " << m_Name
        << ')';
}

void osc::mi::Mesh::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
