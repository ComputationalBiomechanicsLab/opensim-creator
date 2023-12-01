#include "Mesh.hpp"

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIClass.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Utils/UID.hpp>

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
    return TransformAABB(m_MeshData.getBounds(), m_Transform);
}

MIClass osc::mi::Mesh::CreateClass()
{
    return
    {
        ModelGraphStrings::c_MeshLabel,
        ModelGraphStrings::c_MeshLabelPluralized,
        ModelGraphStrings::c_MeshLabelOptionallyPluralized,
        ICON_FA_CUBE,
        ModelGraphStrings::c_MeshDescription,
    };
}

std::vector<CrossrefDescriptor> osc::mi::Mesh::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, ModelGraphStrings::c_MeshAttachmentCrossrefName, CrossrefDirection::ToParent},
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
