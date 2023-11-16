#include "JointEl.hpp"

#include <OpenSimCreator/Registry/ComponentRegistry.hpp>
#include <OpenSimCreator/Registry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/SpanHelpers.hpp>
#include <oscar/Utils/UID.hpp>

#include <cstddef>
#include <string>

osc::JointEl::JointEl(
    UID id,
    size_t jointTypeIdx,
    std::string const& userAssignedName,  // can be empty
    UID parent,
    UID child,
    Transform const& xform) :

    m_ID{id},
    m_JointTypeIndex{jointTypeIdx},
    m_UserAssignedName{osc::SanitizeToOpenSimComponentName(userAssignedName)},
    m_Parent{parent},
    m_Child{child},
    m_Xform{xform}
{
}

osc::CStringView osc::JointEl::getSpecificTypeName() const
{
    return osc::At(osc::GetComponentRegistry<OpenSim::Joint>(), m_JointTypeIndex).name();
}

void osc::JointEl::implSetLabel(std::string_view sv)
{
    m_UserAssignedName = osc::SanitizeToOpenSimComponentName(sv);
}
