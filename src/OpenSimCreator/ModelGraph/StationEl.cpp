#include "StationEl.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

#include <string>
#include <string_view>

osc::StationEl::StationEl(
    UID id,
    UID attachment,
    Vec3 const& position,
    std::string const& name) :

    m_ID{id},
    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

osc::StationEl::StationEl(
    UID attachment,
    Vec3 const& position,
    std::string const& name) :

    m_Attachment{attachment},
    m_Position{position},
    m_Name{SanitizeToOpenSimComponentName(name)}
{
}

void osc::StationEl::implSetLabel(std::string_view sv)
{
    m_Name = SanitizeToOpenSimComponentName(sv);
}
