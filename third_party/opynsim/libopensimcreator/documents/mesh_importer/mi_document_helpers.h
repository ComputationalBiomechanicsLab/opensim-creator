#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>

#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/utilities/uid.h>

#include <concepts>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace osc
{
    class MiBody;
}
namespace osc
{
    class MiJoint;
}

namespace osc
{
    // returns `true` if `body` participates in any joint in the document
    bool is_a_child_attachment_in_any_joint(const MiDocument&, const MiObject&);

    // returns `true` if an `MiJointMiJoint` is complete b.s.
    bool is_garbage_joint(const MiDocument&, const MiJoint&);

    // returns `true` if a body is indirectly or directly attached to ground
    bool is_body_attached_to_ground(
        const MiDocument&, const MiBody&, std::unordered_set<UID>& previously_visited_joints);

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool is_joint_attached_to_ground(const MiDocument&, const MiJoint&, std::unordered_set<UID>&);

    // returns `true` if the document contains issues and populates the output vector with messages
    bool get_issues(const MiDocument&, std::vector<std::string>&);

    // returns a string representing the subheader of an object
    std::string get_context_menu_sub_header_text(const MiDocument&, const MiObject&);

    // returns true if the given object ('s ID) is in the "selection group" of the parent
    bool is_in_selection_group_of(const MiDocument&, UID parent, UID id);

    template<std::invocable<UID> Consumer>
    void for_each_id_in_selection_group(const MiDocument& doc, UID parent, Consumer f)
    {
        for (const MiObject& obj : doc.iter()) {
            const UID id = obj.get_id();

            if (is_in_selection_group_of(doc, parent, id)) {
                f(id);
            }
        }
    }

    void select_anything_grouped_with(MiDocument&, UID);

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the document
    UID get_station_attachment_parent(const MiDocument&, const MiObject&);

    // points an axis of a given object towards some other object in the document
    void point_axis_towards(MiDocument&, UID, int axis, UID);

    // returns decoration flags for an object in the document
    SceneDecorationFlags compute_flags(
        const MiDocument&, UID id, std::optional<UID> maybe_hover_id = std::nullopt);
}
