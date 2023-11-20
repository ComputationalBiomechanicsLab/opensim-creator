#include "MeshEl.hpp"

#include <OpenSimCreator/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/ModelGraph/CrossrefDirection.hpp>
#include <OpenSimCreator/ModelGraph/SceneElClass.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphStrings.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Utils/FilesystemHelpers.hpp>
#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

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

osc::SceneElClass osc::MeshEl::CreateClass()
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

std::vector<osc::CrossrefDescriptor> osc::MeshEl::implGetCrossReferences() const
{
    return
    {
        {m_Attachment, ModelGraphStrings::c_MeshAttachmentCrossrefName, CrossrefDirection::ToParent},
    };
}

std::ostream& osc::MeshEl::implWriteToStream(std::ostream& o) const
{
    return o << "MeshEl("
        << "ID = " << m_ID
        << ", Attachment = " << m_Attachment
        << ", m_Transform = " << m_Transform
        << ", MeshData = " << &m_MeshData
        << ", Path = " << m_Path
        << ", Name = " << m_Name
        << ')';
}

void osc::MeshEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
