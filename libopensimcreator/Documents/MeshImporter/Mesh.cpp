#include "Mesh.h"

#include <libopensimcreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <libopensimcreator/Documents/MeshImporter/CrossrefDirection.h>
#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIStrings.h>
#include <libopensimcreator/Platform/msmicons.h>

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/utils/uid.h>

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

std::optional<AABB> osc::mi::Mesh::calcBounds() const
{
    return m_MeshData.bounds().transform([this](const AABB& localBounds)
    {
        return transform_aabb(m_Transform, localBounds);
    });
}

void osc::mi::Mesh::reloadMeshDataFromDisk()
{
    m_MeshData = LoadMeshViaSimbody(getPath());
}

MIClass osc::mi::Mesh::CreateClass()
{
    return
    {
        MIStrings::c_MeshLabel,
        MIStrings::c_MeshLabelPluralized,
        MIStrings::c_MeshLabelOptionallyPluralized,
        MSMICONS_CUBE,
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
