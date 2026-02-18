#include "mi_mesh.h"

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/utilities/uid.h>

#include <filesystem>
#include <ostream>
#include <utility>
#include <vector>

using osc::MiCrossrefDescriptor;
using osc::MiClass;
using osc::AABB;

osc::MiMesh::MiMesh(
    UID id,
    UID attachment,
    osc::Mesh meshData,
    std::filesystem::path path) :

    m_ID{id},
    m_Attachment{attachment},
    m_MeshData{std::move(meshData)},
    m_Path{std::move(path)},
    m_Name{opyn::SanitizeToOpenSimComponentName(m_Path.filename().replace_extension().string())}
{}

std::optional<AABB> osc::MiMesh::calcBounds() const
{
    return m_MeshData.bounds().transform([this](const AABB& localBounds)
    {
        return transform_aabb(m_Transform, localBounds);
    });
}

void osc::MiMesh::reloadMeshDataFromDisk()
{
    m_MeshData = opyn::LoadMeshViaSimbody(getPath());
}

MiClass osc::MiMesh::CreateClass()
{
    return
    {
        MiStrings::c_MeshLabel,
        MiStrings::c_MeshLabelPluralized,
        MiStrings::c_MeshLabelOptionallyPluralized,
        MSMICONS_CUBE,
        MiStrings::c_MeshDescription,
    };
}

std::vector<MiCrossrefDescriptor> osc::MiMesh::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, MiStrings::c_MeshAttachmentCrossrefName, MiCrossrefDirection::ToParent},
    };
}

std::ostream& osc::MiMesh::implWriteToStream(std::ostream& o) const
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

void osc::MiMesh::implSetLabel(std::string_view sv)
{
    m_Name = opyn::SanitizeToOpenSimComponentName(sv);
}
