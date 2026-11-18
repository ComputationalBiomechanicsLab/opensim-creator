#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_undoable_document.h>

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/uid.h>

#include <optional>
#include <span>
#include <string>
#include <unordered_set>

namespace osc
{
    class MiObject;
}

// undoable action support
//
// functions that mutate the undoable data structure and commit changes at the
// correct time
namespace osc
{
    bool point_axis_towards(MiUndoableDocument&, UID id, int axis, UID other);

    bool try_assign_mesh_attachments(
        MiUndoableDocument&, const std::unordered_set<UID>& mesh_ids, UID new_attachment);

    bool try_create_joint(MiUndoableDocument&, UID child_id, UID parent_id);

    bool try_orient_object_axis_along_two_points(
        MiUndoableDocument&, UID id, int axis, Vector3 p1, Vector3 p2);

    bool try_orient_object_axis_along_two_objects(
        MiUndoableDocument&, UID id, int axis, UID obj1, UID obj2);

    bool try_translate_object_between_two_points(
        MiUndoableDocument&, UID id, const Vector3&, const Vector3&);

    bool try_translate_between_two_objects(MiUndoableDocument&, UID id, UID a, UID b);

    bool try_translate_object_to_another_object(MiUndoableDocument&, UID id, UID other);

    bool try_translate_to_mesh_average_center(MiUndoableDocument&, UID id, UID mesh_id);

    bool try_translate_to_mesh_bounds_center(MiUndoableDocument&, UID id, UID mesh_id);

    bool try_translate_to_mesh_mass_center(MiUndoableDocument&, UID id, UID mesh_id);

    bool try_reassign_crossref(MiUndoableDocument&, UID id, int crossref, UID other);

    bool delete_selected(MiUndoableDocument&);

    bool delete_object(MiUndoableDocument&, UID id);

    void rotate_axis(MiUndoableDocument&, MiObject& el, int axis, Radians radians);

    bool try_copy_orientation(MiUndoableDocument&, UID id, UID other);

    UID add_body(MiUndoableDocument&, const Vector3&, UID and_try_attach);

    UID add_body(MiUndoableDocument&);

    bool add_station_at_location(MiUndoableDocument&, const MiObject& obj, const Vector3&);

    bool add_station_at_location(MiUndoableDocument&, UID attachment, const Vector3&);

    void action_import_landmarks(
        MiUndoableDocument&,
        std::span<const opyn::NamedLandmark>,
        std::optional<std::string> maybe_name = std::nullopt);
}
