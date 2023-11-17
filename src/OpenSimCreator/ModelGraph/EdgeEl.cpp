#include "EdgeEl.hpp"

#include <OpenSimCreator/ModelGraph/ISceneElLookup.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>
#include <OpenSimCreator/ModelGraph/SceneElClass.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <iostream>

osc::SceneElClass osc::EdgeEl::CreateClass()
{
    return SceneElClass
    {
        "Edge",
        "Edges",
        "Edge(s)",
        ICON_FA_ARROWS_ALT,
        "An edge between the centers of two other scene elements",
    };
}

osc::Transform osc::EdgeEl::implGetXform(ISceneElLookup const& lookup) const
{
    SceneEl const* first = lookup.find(m_FirstAttachmentID);
    SceneEl const* second = lookup.find(m_FirstAttachmentID);
    if (first && second)
    {
        Vec3 const pos = osc::Midpoint(first->getPos(lookup), second->getPos(lookup));
        return Transform{.position = pos};
    }
    else
    {
        return Identity<Transform>();
    }
}

std::ostream& osc::EdgeEl::implWriteToStream(std::ostream& out) const
{
    return out << "Edge(id = " << m_ID << ", lhs = " << m_FirstAttachmentID << ", rhs = " << m_SecondAttachmentID << ')';
}

osc::AABB osc::EdgeEl::implCalcBounds(ISceneElLookup const& lookup) const
{
    SceneEl const* first = lookup.find(m_FirstAttachmentID);
    SceneEl const* second = lookup.find(m_FirstAttachmentID);
    if (first && second)
    {
        return osc::BoundingAABBOf(std::to_array(
        {
            first->getPos(lookup),
            second->getPos(lookup),
        }));
    }
    else
    {
        return AABB{};
    }
}
