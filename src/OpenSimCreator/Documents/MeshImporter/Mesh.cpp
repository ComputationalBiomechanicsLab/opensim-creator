#include "Mesh.h"

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.h>
#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Utils/UID.h>

#include <filesystem>
#include <iostream>
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
{
}

AABB osc::mi::Mesh::calcBounds() const
{
    return transform_aabb(m_MeshData.getBounds(), m_Transform);
}

MIClass osc::mi::Mesh::CreateClass()
{
    return
    {
        MIStrings::c_MeshLabel,
        MIStrings::c_MeshLabelPluralized,
        MIStrings::c_MeshLabelOptionallyPluralized,
        ICON_FA_CUBE,
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
