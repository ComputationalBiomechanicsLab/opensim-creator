#include "BodyEl.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/UID.hpp>

#include <string>
#include <string_view>

osc::BodyEl::BodyEl(
    UID id,
    std::string const& name,
    Transform const& xform) :

    m_ID{id},
    m_Name{SanitizeToOpenSimComponentName(name)},
    m_Xform{xform}
{
}

void osc::BodyEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
