#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/documents/mesh_warper/mw_undoable_document.h>

#include <libopynsim/documents/landmarks/landmark_csv_flags.h>
#include <liboscar/formats/obj.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_set>

// user-enactable actions
namespace osc
{
    class Mesh;
    class MwResultCache;
    struct MwDocument;
    struct MwDocumentLandmarkPair;

    // adds a landmark to an input mesh
    void action_add_landmark(MwUndoableDocument&, MiDocumentInputIdentifier, const Vector3&);

    // adds a non-participating landmark to the source mesh
    void action_add_non_participating_landmark(MwUndoableDocument&, const Vector3&);

    // adds a source/destination position to an existing landmark
    void action_set_landmark_position(
        MwUndoableDocument&, UID, MiDocumentInputIdentifier, const Vector3&);

    // tries to rename the landmark to `newName`, returns the actual new name (even if it hasn't
    // changed)
    void action_rename_landmark(MwUndoableDocument&, UID, std::string_view new_name);

    // sets the IDed non-participating landmark's location to the given location
    void action_set_non_participating_landmark_position(MwUndoableDocument&, UID, const Vector3&);

    // tries to rename the non-participating landmark to `newName`, returns the actual new name
    // (even if it hasn't changed)
    void action_rename_non_participating_landmark(
        MwUndoableDocument&, UID, std::string_view new_name);

    // sets the TPS blending factor for the result, but does not save the change to undo/redo
    // storage
    void action_set_blend_factor_without_committing(MwUndoableDocument&, float factor);

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void action_set_blend_factor(MwUndoableDocument&, float factor);

    // sets whether the engine should recalculate the mesh's normal after applying the warp
    void action_set_recalculating_normals(MwUndoableDocument&, bool new_state);

    // sets the pending penalty of the TPS warp
    void action_set_warping_penalty(MwUndoableDocument&, float new_warping_penalty);

    // sets the source landmark prescale for the mesh warper
    void action_set_source_landmarks_prescale(
        MwUndoableDocument&, float new_source_landmarks_prescale);

    // sets the destination landmark prescale for the mesh warper
    void action_set_destination_landmarks_prescale(
        MwUndoableDocument&, float new_destination_landmarks_prescale);

    // creates a "fresh" (default) TPS document
    void action_create_new_document(MwUndoableDocument&);

    // clears all user-assigned landmarks in the TPS document
    void action_clear_all_landmarks(MwUndoableDocument&);

    // clears all non-participating landmarks in the TPS document
    void action_clear_all_non_participating_landmarks(MwUndoableDocument&);

    // deletes the specified landmarks from the TPS document
    void action_delete_scene_elements_by_id(
        MwUndoableDocument&, const std::unordered_set<MwDocumentElementID>&);
    void action_delete_element_by_id(MwUndoableDocument&, UID);

    // assigns the given mesh to the document
    void action_load_mesh(MwUndoableDocument&, const Mesh&, MiDocumentInputIdentifier);

    // prompts the user to browse for an input mesh and assigns it to the document
    void action_prompt_user_to_load_mesh_file(
        const std::shared_ptr<MwUndoableDocument>&, MiDocumentInputIdentifier);

    // loads landmarks from a CSV file into source/destination slot of the document
    void action_prompt_user_to_load_landmarks_from_csv(
        const std::shared_ptr<MwUndoableDocument>&, MiDocumentInputIdentifier);

    // loads non-participating landmarks from a CSV file into the source input
    void action_prompt_user_to_load_non_participating_landmarks_from_csv(
        const std::shared_ptr<MwUndoableDocument>&);

    // saves all source/destination landmarks to a CSV file (matches loading)
    void action_prompt_user_to_save_landmarks_to_csv(
        const MwDocument&,
        MiDocumentInputIdentifier,
        opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // writes all source/destination landmark pairs with a location to the output stream in a CSV
    // format.
    void action_write_landmarks_as_csv(
        std::span<const MwDocumentLandmarkPair>,
        MiDocumentInputIdentifier,
        opyn::LandmarkCSVFlags,
        std::ostream&);

    // saves non-participating landmarks to a CSV file (matches loading)
    void action_prompt_user_to_save_non_participating_landmarks_to_csv(
        const MwDocument&, opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void action_prompt_user_to_save_paired_landmarks_to_csv(
        const MwDocument&, opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // prompts the user to save the mesh to an obj file
    void action_prompt_user_to_save_mesh_to_obj_file(const Mesh&, OBJWriterFlags);

    // prompts the user to save the mesh to an stl file
    void action_prompt_user_to_mesh_to_stl_file(const Mesh&);

    // prompts the user to save the (already warped) points to a CSV file
    void action_prompt_user_to_save_warped_non_participating_landmarks_to_csv(
        const MwDocument&, MwResultCache&, opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // swaps the source and destination (incl. premultiply, etc.)
    void action_swap_source_destination(MwUndoableDocument&);

    // translate the chosen landmarks by a translation vector, but don't save the undoable output
    void action_translate_landmarks_dont_save(
        MwUndoableDocument&,
        const std::unordered_set<MwDocumentElementID>& landmark_ids,
        const Vector3& translation);

    // commit/save the scratch space with a "translated landmarks" message (pair this with
    // `ActionTranslateLandmarksDontSave`)
    void action_save_landmark_translation(
        MwUndoableDocument&, const std::unordered_set<MwDocumentElementID>& landmark_ids);

    // moves the chosen landmarks to `location` and saves the scratch space with a "set landmark
    // locations" message
    void action_set_landmark_locations(
        MwUndoableDocument&,
        const std::unordered_set<MwDocumentElementID>&,
        const Vector3& location);
}
