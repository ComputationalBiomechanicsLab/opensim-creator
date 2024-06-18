#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>

#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Utils/UID.h>

#include <concepts>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace osc::mi { class Body; }
namespace osc::mi { class Joint; }

namespace osc::mi
{
    // returns `true` if `body` participates in any joint in the document
    bool IsAChildAttachmentInAnyJoint(const Document&, const MIObject&);

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(const Document&, const Joint&);

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(
        const Document&,
        const Body&,
        std::unordered_set<UID>& previouslyVisitedJoints
    );

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(
        const Document&,
        const Joint&,
        std::unordered_set<UID>&
    );

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(
        const Document&,
        const Body&,
        std::unordered_set<UID>&
    );

    // returns `true` if the document contains issues and populates the output vector with messages
    bool GetIssues(
        const Document&,
        std::vector<std::string>&
    );

    // returns a string representing the subheader of an object
    std::string GetContextMenuSubHeaderText(
        const Document&,
        const MIObject&
    );

    // returns true if the given object ('s ID) is in the "selection group" of the parent
    bool IsInSelectionGroupOf(
        const Document&,
        UID parent,
        UID id
    );

    template<std::invocable<UID> Consumer>
    void ForEachIDInSelectionGroup(
        const Document& doc,
        UID parent,
        Consumer f)
    {
        for (const MIObject& obj : doc.iter())
        {
            const UID id = obj.getID();

            if (IsInSelectionGroupOf(doc, parent, id))
            {
                f(id);
            }
        }
    }

    void SelectAnythingGroupedWith(Document&, UID);

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the document
    UID GetStationAttachmentParent(const Document&, const MIObject&);

    // points an axis of a given object towards some other object in the document
    void point_axis_towards(
        Document&,
        UID,
        int axis,
        UID
    );

    // returns decoration flags for an object in the document
    SceneDecorationFlags computeFlags(
        const Document&,
        UID id,
        std::optional<UID> maybeHoverID = std::nullopt
    );
}
