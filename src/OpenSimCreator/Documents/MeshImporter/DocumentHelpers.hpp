#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.hpp>

#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <string>
#include <optional>
#include <unordered_set>
#include <vector>

namespace osc::mi { class Body; }
namespace osc::mi { class Joint; }

namespace osc::mi
{
    void SelectOnly(Document&, MIObject const&);
    bool HasSelection(Document const&);
    void DeleteSelected(Document&);
    CStringView getLabel(Document const&, UID);
    Transform GetTransform(Document const&, UID);
    Vec3 GetPosition(Document const&, UID);

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(Document const&, MIObject const&);

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(Document const&, Joint const&);

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(
        Document const&,
        Body const&,
        std::unordered_set<UID>& previouslyVisitedJoints
    );

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(
        Document const&,
        Joint const&,
        std::unordered_set<UID>&
    );

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(
        Document const&,
        Body const&,
        std::unordered_set<UID>&
    );

    // returns `true` if `modelGraph` contains issues
    bool GetModelGraphIssues(
        Document const&,
        std::vector<std::string>&
    );

    // returns a string representing the subheader of a scene element
    std::string GetContextMenuSubHeaderText(
        Document const& mg,
        MIObject const&
    );

    // returns true if the given element (ID) is in the "selection group" of
    bool IsInSelectionGroupOf(
        Document const&,
        UID parent,
        UID id
    );

    template<typename Consumer>
    void ForEachIDInSelectionGroup(
        Document const& mg,
        UID parent,
        Consumer f)
        requires Invocable<Consumer, UID>
    {
        for (MIObject const& e : mg.iter())
        {
            UID const id = e.getID();

            if (IsInSelectionGroupOf(mg, parent, id))
            {
                f(id);
            }
        }
    }

    void SelectAnythingGroupedWith(Document&, UID);

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the scene
    UID GetStationAttachmentParent(Document const&, MIObject const&);

    // points an axis of a given element towards some other element in the model graph
    void PointAxisTowards(
        Document&,
        UID,
        int axis,
        UID
    );

    // returns recommended rim intensity for an element in the model graph
    SceneDecorationFlags computeFlags(
        Document const&,
        UID id,
        std::optional<UID> maybeHoverID = std::nullopt
    );
}
