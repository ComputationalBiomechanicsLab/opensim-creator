#include "DocumentHelpers.h"

#include <OpenSimCreator/Documents/MeshImporter/Body.h>
#include <OpenSimCreator/Documents/MeshImporter/Document.h>
#include <OpenSimCreator/Documents/MeshImporter/Joint.h>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.h>
#include <OpenSimCreator/Documents/MeshImporter/Station.h>

#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/UID.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

using namespace osc;

bool osc::mi::IsAChildAttachmentInAnyJoint(Document const& doc, MIObject const& obj)
{
    auto const iterable = doc.iter<Joint>();
    return std::any_of(iterable.begin(), iterable.end(), [id = obj.getID()](Joint const& j)
    {
        return j.getChildID() == id;
    });
}

bool osc::mi::IsGarbageJoint(Document const& doc, Joint const& joint)
{
    if (joint.getChildID() == MIIDs::Ground())
    {
        return true;  // ground cannot be a child in a joint
    }

    if (joint.getParentID() == joint.getChildID())
    {
        return true;  // is directly attached to itself
    }

    if (joint.getParentID() != MIIDs::Ground() && !doc.contains<Body>(joint.getParentID()))
    {
        return true;  // has a parent ID that's invalid for this document
    }

    if (!doc.contains<Body>(joint.getChildID()))
    {
        return true;  // has a child ID that's invalid for this document
    }

    return false;
}

bool osc::mi::IsJointAttachedToGround(
    Document const& doc,
    Joint const& joint,
    std::unordered_set<UID>& previousVisits)
{
    OSC_ASSERT_ALWAYS(!IsGarbageJoint(doc, joint));

    if (joint.getParentID() == MIIDs::Ground())
    {
        return true;  // it's directly attached to ground
    }

    auto const* const parent = doc.tryGetByID<Body>(joint.getParentID());
    if (!parent)
    {
        return false;  // joint's parent is garbage
    }

    // else: recurse to parent
    return IsBodyAttachedToGround(doc, *parent, previousVisits);
}

bool osc::mi::IsBodyAttachedToGround(
    Document const& doc,
    Body const& body,
    std::unordered_set<UID>& previouslyVisitedJoints)
{
    bool childInAtLeastOneJoint = false;

    for (Joint const& joint : doc.iter<Joint>())
    {
        OSC_ASSERT(!IsGarbageJoint(doc, joint));

        if (joint.getChildID() == body.getID())
        {
            childInAtLeastOneJoint = true;

            bool const alreadyVisited = !previouslyVisitedJoints.emplace(joint.getID()).second;
            if (alreadyVisited)
            {
                continue;  // skip this joint: was previously visited
            }

            if (IsJointAttachedToGround(doc, joint, previouslyVisitedJoints))
            {
                return true;  // recurse
            }
        }
    }

    return !childInAtLeastOneJoint;
}

bool osc::mi::GetIssues(
    Document const& doc,
    std::vector<std::string>& issuesOut)
{
    issuesOut.clear();

    for (Joint const& joint : doc.iter<Joint>())
    {
        if (IsGarbageJoint(doc, joint))
        {
            std::stringstream ss;
            ss << joint.getLabel() << ": joint is garbage (this is an implementation error)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    for (Body const& body : doc.iter<Body>())
    {
        std::unordered_set<UID> previouslyVisitedJoints;
        if (!IsBodyAttachedToGround(doc, body, previouslyVisitedJoints))
        {
            std::stringstream ss;
            ss << body.getLabel() << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
            issuesOut.push_back(std::move(ss).str());
        }
    }

    return !issuesOut.empty();
}

std::string osc::mi::GetContextMenuSubHeaderText(
    Document const& doc,
    MIObject const& obj)
{
    std::stringstream ss;
    std::visit(Overload
    {
        [&ss](Ground const&)
        {
            ss << "(scene origin)";
        },
        [&ss, &doc](Mesh const& m)
        {
            ss << '(' << m.getClass().getName() << ", " << m.getPath().filename().string() << ", attached to " << doc.getLabelByID(m.getParentID()) << ')';
        },
        [&ss](Body const& b)
        {
            ss << '(' << b.getClass().getName() << ')';
        },
        [&ss, &doc](Joint const& j)
        {
            ss << '(' << j.getSpecificTypeName() << ", " << doc.getLabelByID(j.getChildID()) << " --> " << doc.getLabelByID(j.getParentID()) << ')';
        },
        [&ss, &doc](StationEl const& s)
        {
            ss << '(' << s.getClass().getName() << ", attached to " << doc.getLabelByID(s.getParentID()) << ')';
        },
    }, obj.toVariant());
    return std::move(ss).str();
}

bool osc::mi::IsInSelectionGroupOf(
    Document const& doc,
    UID parent,
    UID id)
{
    if (id == MIIDs::Empty() || parent == MIIDs::Empty())
    {
        return false;
    }

    if (id == parent)
    {
        return true;
    }

    Body const* body = nullptr;
    if (auto const* be = doc.tryGetByID<Body>(parent))
    {
        body = be;
    }
    else if (auto const* me = doc.tryGetByID<Mesh>(parent))
    {
        body = doc.tryGetByID<Body>(me->getParentID());
    }

    if (!body)
    {
        return false;  // parent isn't attached to any body (or isn't a body)
    }

    if (auto const* be = doc.tryGetByID<Body>(id))
    {
        return be->getID() == body->getID();
    }
    else if (auto const* me = doc.tryGetByID<Mesh>(id))
    {
        return me->getParentID() == body->getID();
    }
    else
    {
        return false;
    }
}

void osc::mi::SelectAnythingGroupedWith(Document& doc, UID id)
{
    ForEachIDInSelectionGroup(doc, id, [&doc](UID other)
    {
        doc.select(other);
    });
}

UID osc::mi::GetStationAttachmentParent(Document const& doc, MIObject const& obj)
{
    return std::visit(Overload
    {
        [](Ground const&) { return MIIDs::Ground(); },
        [&doc](Mesh const& meshEl) { return doc.contains<Body>(meshEl.getParentID()) ? meshEl.getParentID() : MIIDs::Ground(); },
        [](Body const& bodyEl) { return bodyEl.getID(); },
        [](Joint const&) { return MIIDs::Ground(); },
        [](StationEl const&) { return MIIDs::Ground(); },
    }, obj.toVariant());
}

void osc::mi::PointAxisTowards(
    Document& doc,
    UID id,
    int axis,
    UID other)
{
    Vec3 const choicePos = doc.getPosByID(other);
    Transform const sourceXform = Transform{.position = doc.getPosByID(id)};

    doc.updByID(id).setXform(doc, PointAxisTowards(sourceXform, axis, choicePos));
}

SceneDecorationFlags osc::mi::computeFlags(
    Document const& doc,
    UID id,
    std::optional<UID> maybeHoverID)
{
    UID const hoverID = maybeHoverID ? *maybeHoverID : MIIDs::Empty();

    if (id == MIIDs::Empty())
    {
        return SceneDecorationFlags::None;
    }
    else if (doc.isSelected(id))
    {
        return SceneDecorationFlags::IsSelected;
    }
    else if (id == hoverID)
    {
        return SceneDecorationFlags::IsHovered | SceneDecorationFlags::IsChildOfHovered;
    }
    else if (IsInSelectionGroupOf(doc, hoverID, id))
    {
        return SceneDecorationFlags::IsChildOfHovered;
    }
    else
    {
        return SceneDecorationFlags::None;
    }
}
