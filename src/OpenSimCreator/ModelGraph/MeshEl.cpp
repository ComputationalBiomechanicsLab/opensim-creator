#include "MeshEl.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Utils/FilesystemHelpers.hpp>
#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <utility>

osc::MeshEl::MeshEl(
    UID id,
    UID attachment,
    Mesh meshData,
    std::filesystem::path path) :

    m_ID{id},
    m_Attachment{attachment},
    m_MeshData{std::move(meshData)},
    m_Path{std::move(path)},
    m_Name{SanitizeToOpenSimComponentName(FileNameWithoutExtension(m_Path))}
{
}

osc::AABB osc::MeshEl::calcBounds() const
{
    return TransformAABB(m_MeshData.getBounds(), m_Transform);
}

void osc::MeshEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
