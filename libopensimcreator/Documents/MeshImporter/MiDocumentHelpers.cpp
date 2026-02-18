#include "MiDocumentHelpers.h"

#include <libopensimcreator/Documents/MeshImporter/MiBody.h>
#include <libopensimcreator/Documents/MeshImporter/MiDocument.h>
#include <libopensimcreator/Documents/MeshImporter/MiJoint.h>
#include <libopensimcreator/Documents/MeshImporter/MiIDs.h>
#include <libopensimcreator/Documents/MeshImporter/MiMesh.h>
#include <libopensimcreator/Documents/MeshImporter/MiObject.h>
#include <libopensimcreator/Documents/MeshImporter/MiStation.h>

#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/std_variant_helpers.h>
#include <liboscar/utilities/uid.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

using namespace osc;

bool osc::IsAChildAttachmentInAnyJoint(const MiDocument& doc, const MiObject& obj)
{
    return cpp23::contains(doc.iter<MiJoint>(), obj.getID(), &MiJoint::getChildID);
}

bool osc::IsGarbageJoint(const MiDocument& doc, const MiJoint& joint)
{
    if (joint.getChildID() == MiIDs::Ground())
    {
        return true;  // ground cannot be a child in a joint
    }

    if (joint.getParentID() == joint.getChildID())
    {
        return true;  // is directly attached to itself
    }

    if (joint.getParentID() != MiIDs::Ground() && !doc.contains<MiBody>(joint.getParentID()))
    {
        return true;  // has a parent ID that's invalid for this document
    }

    if (!doc.contains<MiBody>(joint.getChildID()))
    {
        return true;  // has a child ID that's invalid for this document
    }

    return false;
}

bool osc::IsJointAttachedToGround(
    const MiDocument& doc,
    const MiJoint& joint,
    std::unordered_set<UID>& previousVisits)
{
    OSC_ASSERT_ALWAYS(!IsGarbageJoint(doc, joint));

    if (joint.getParentID() == MiIDs::Ground())
    {
        return true;  // it's directly attached to ground
    }

    const auto* const parent = doc.tryGetByID<MiBody>(joint.getParentID());
    if (!parent)
    {
        return false;  // joint's parent is garbage
    }

    // else: recurse to parent
    return IsBodyAttachedToGround(doc, *parent, previousVisits);
}

bool osc::IsBodyAttachedToGround(
    const MiDocument& doc,
    const MiBody& body,
    std::unordered_set<UID>& previouslyVisitedJoints)
{
    bool childInAtLeastOneJoint = false;

    for (const MiJoint& joint : doc.iter<MiJoint>())
    {
        OSC_ASSERT(!IsGarbageJoint(doc, joint));

        if (joint.getChildID() == body.getID())
        {
            childInAtLeastOneJoint = true;

            const bool alreadyVisited = !previouslyVisitedJoints.emplace(joint.getID()).second;
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

bool osc::GetIssues(
    const MiDocument& doc,
    std::vector<std::string>& issuesOut)
{
    issuesOut.clear();

    for (const MiJoint& joint : doc.iter<MiJoint>())
    {
        if (IsGarbageJoint(doc, joint))
        {
            std::stringstream ss;
            ss << joint.getLabel() << ": joint is garbage (this is an implementation error)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    for (const MiBody& body : doc.iter<MiBody>())
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

std::string osc::GetContextMenuSubHeaderText(
    const MiDocument& doc,
    const MiObject& obj)
{
    std::stringstream ss;
    std::visit(Overload
    {
        [&ss](const MiGround&)
        {
            ss << "(scene origin)";
        },
        [&ss, &doc](const MiMesh& m)
        {
            ss << '(' << m.getClass().getName() << ", " << m.getPath().filename().string() << ", attached to " << doc.getLabelByID(m.getParentID()) << ')';
        },
        [&ss](const MiBody& b)
        {
            ss << '(' << b.getClass().getName() << ')';
        },
        [&ss, &doc](const MiJoint& j)
        {
            ss << '(' << j.getSpecificTypeName() << ", " << doc.getLabelByID(j.getChildID()) << " --> " << doc.getLabelByID(j.getParentID()) << ')';
        },
        [&ss, &doc](const MiStation& s)
        {
            ss << '(' << s.getClass().getName() << ", attached to " << doc.getLabelByID(s.getParentID()) << ')';
        },
    }, obj.toVariant());
    return std::move(ss).str();
}

bool osc::IsInSelectionGroupOf(
    const MiDocument& doc,
    UID parent,
    UID id)
{
    if (id == MiIDs::Empty() || parent == MiIDs::Empty())
    {
        return false;
    }

    if (id == parent)
    {
        return true;
    }

    const MiBody* body = nullptr;
    if (const auto* be = doc.tryGetByID<MiBody>(parent))
    {
        body = be;
    }
    else if (const auto* me = doc.tryGetByID<MiMesh>(parent))
    {
        body = doc.tryGetByID<MiBody>(me->getParentID());
    }

    if (!body)
    {
        return false;  // parent isn't attached to any body (or isn't a body)
    }

    if (const auto* be = doc.tryGetByID<MiBody>(id))
    {
        return be->getID() == body->getID();
    }
    else if (const auto* me = doc.tryGetByID<MiMesh>(id))
    {
        return me->getParentID() == body->getID();
    }
    else
    {
        return false;
    }
}

void osc::SelectAnythingGroupedWith(MiDocument& doc, UID id)
{
    ForEachIDInSelectionGroup(doc, id, [&doc](UID other)
    {
        doc.select(other);
    });
}

UID osc::GetStationAttachmentParent(const MiDocument& doc, const MiObject& obj)
{
    return std::visit(Overload
    {
        [](const MiGround&) { return MiIDs::Ground(); },
        [&doc](const MiMesh& meshEl) { return doc.contains<MiBody>(meshEl.getParentID()) ? meshEl.getParentID() : MiIDs::Ground(); },
        [](const MiBody& bodyEl) { return bodyEl.getID(); },
        [](const MiJoint&) { return MiIDs::Ground(); },
        [](const MiStation&) { return MiIDs::Ground(); },
    }, obj.toVariant());
}

void osc::point_axis_towards(
    MiDocument& doc,
    UID id,
    int axis,
    UID other)
{
    const Vector3 choicePos = doc.getPosByID(other);
    const Transform sourceXform = {.translation = doc.getPosByID(id)};

    doc.updByID(id).setXform(doc, point_axis_towards(sourceXform, axis, choicePos));
}

SceneDecorationFlags osc::computeFlags(
    const MiDocument& doc,
    UID id,
    std::optional<UID> maybeHoverID)
{
    if (id == MiIDs::Empty()) {
        return SceneDecorationFlag::Default;
    }

    const UID hoverID = maybeHoverID ? *maybeHoverID : MiIDs::Empty();
    SceneDecorationFlags rv = SceneDecorationFlag::Default;
    if (doc.isSelected(id)) {
        rv |= SceneDecorationFlag::RimHighlight0;
    }
    if (id == hoverID or IsInSelectionGroupOf(doc, hoverID, id)) {
        rv |= SceneDecorationFlag::RimHighlight1;
    }
    return rv;
}
