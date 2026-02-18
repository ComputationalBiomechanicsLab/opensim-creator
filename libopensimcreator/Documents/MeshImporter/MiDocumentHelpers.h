#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiDocument.h>
#include <libopensimcreator/Documents/MeshImporter/MiObject.h>

#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/utilities/uid.h>

#include <concepts>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace osc { class MiBody; }
namespace osc { class MiJoint; }

namespace osc
{
    // returns `true` if `body` participates in any joint in the document
    bool IsAChildAttachmentInAnyJoint(const MiDocument&, const MiObject&);

    // returns `true` if an `MiJointMiJoint` is complete b.s.
    bool IsGarbageJoint(const MiDocument&, const MiJoint&);

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(
        const MiDocument&,
        const MiBody&,
        std::unordered_set<UID>& previouslyVisitedJoints
    );

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(
        const MiDocument&,
        const MiJoint&,
        std::unordered_set<UID>&
    );

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(
        const MiDocument&,
        const MiBody&,
        std::unordered_set<UID>&
    );

    // returns `true` if the document contains issues and populates the output vector with messages
    bool GetIssues(
        const MiDocument&,
        std::vector<std::string>&
    );

    // returns a string representing the subheader of an object
    std::string GetContextMenuSubHeaderText(
        const MiDocument&,
        const MiObject&
    );

    // returns true if the given object ('s ID) is in the "selection group" of the parent
    bool IsInSelectionGroupOf(
        const MiDocument&,
        UID parent,
        UID id
    );

    template<std::invocable<UID> Consumer>
    void ForEachIDInSelectionGroup(
        const MiDocument& doc,
        UID parent,
        Consumer f)
    {
        for (const MiObject& obj : doc.iter())
        {
            const UID id = obj.getID();

            if (IsInSelectionGroupOf(doc, parent, id))
            {
                f(id);
            }
        }
    }

    void SelectAnythingGroupedWith(MiDocument&, UID);

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the document
    UID GetStationAttachmentParent(const MiDocument&, const MiObject&);

    // points an axis of a given object towards some other object in the document
    void point_axis_towards(
        MiDocument&,
        UID,
        int axis,
        UID
    );

    // returns decoration flags for an object in the document
    SceneDecorationFlags computeFlags(
        const MiDocument&,
        UID id,
        std::optional<UID> maybeHoverID = std::nullopt
    );
}
