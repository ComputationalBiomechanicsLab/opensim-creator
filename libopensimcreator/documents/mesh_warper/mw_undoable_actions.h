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

namespace osc { class Mesh; }
namespace osc { class MwResultCache; }
namespace osc { struct MwDocument; }
namespace osc { struct MwDocumentLandmarkPair; }

// user-enactable actions
namespace osc
{
    // adds a landmark to an input mesh
    void ActionAddLandmark(MwUndoableDocument&, MiDocumentInputIdentifier, const Vector3&);

    // adds a non-participating landmark to the source mesh
    void ActionAddNonParticipatingLandmark(MwUndoableDocument&, const Vector3&);

    // adds a source/destination position to an existing landmark
    void ActionSetLandmarkPosition(MwUndoableDocument&, UID, MiDocumentInputIdentifier, const Vector3&);

    // tries to rename the landmark to `newName`, returns the actual new name (even if it hasn't changed)
    void ActionRenameLandmark(MwUndoableDocument&, UID, std::string_view newName);

    // sets the IDed non-participating landmark's location to the given location
    void ActionSetNonParticipatingLandmarkPosition(MwUndoableDocument&, UID, const Vector3&);

    // tries to rename the non-particiapting landmark to `newName`, returns the actual new name (even if it hasn't changed)
    void ActionRenameNonParticipatingLandmark(MwUndoableDocument&, UID, std::string_view newName);

    // sets the TPS blending factor for the result, but does not save the change to undo/redo storage
    void ActionSetBlendFactorWithoutCommitting(MwUndoableDocument&, float factor);

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void ActionSetBlendFactor(MwUndoableDocument&, float factor);

    // sets whether the engine should recalculate the mesh's normal after applying the warp
    void ActionSetRecalculatingNormals(MwUndoableDocument&, bool newState);

    // sets the source landmark prescale for the mesh warper
    void ActionSetSourceLandmarksPrescale(MwUndoableDocument&, float newSourceLandmarksPrescale);

    // sets the destination landmark prescale for the mesh warper
    void ActionSetDestinationLandmarksPrescale(MwUndoableDocument&, float newDestinationLandmarksPrescale);

    // creates a "fresh" (default) TPS document
    void ActionCreateNewDocument(MwUndoableDocument&);

    // clears all user-assigned landmarks in the TPS document
    void ActionClearAllLandmarks(MwUndoableDocument&);

    // clears all non-participating landmarks in the TPS document
    void ActionClearAllNonParticipatingLandmarks(MwUndoableDocument&);

    // deletes the specified landmarks from the TPS document
    void ActionDeleteSceneElementsByID(MwUndoableDocument&, const std::unordered_set<MwDocumentElementID>&);
    void ActionDeleteElementByID(MwUndoableDocument&, UID);

    // assigns the given mesh to the document
    void ActionLoadMesh(MwUndoableDocument&, const Mesh&, MiDocumentInputIdentifier);

    // prompts the user to browse for an input mesh and assigns it to the document
    void ActionPromptUserToLoadMeshFile(const std::shared_ptr<MwUndoableDocument>&, MiDocumentInputIdentifier);

    // loads landmarks from a CSV file into source/destination slot of the document
    void ActionPromptUserToLoadLandmarksFromCSV(const std::shared_ptr<MwUndoableDocument>&, MiDocumentInputIdentifier);

    // loads non-participating landmarks from a CSV file into the source input
    void ActionPromptUserToLoadNonParticipatingLandmarksFromCSV(const std::shared_ptr<MwUndoableDocument>&);

    // saves all source/destination landmarks to a CSV file (matches loading)
    void ActionPromptUserToSaveLandmarksToCSV(const MwDocument&, MiDocumentInputIdentifier, opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // writes all source/destination landmark pairs with a location to the output stream in a CSV format.
    void ActionWriteLandmarksAsCSV(std::span<const MwDocumentLandmarkPair>, MiDocumentInputIdentifier, opyn::LandmarkCSVFlags, std::ostream&);

    // saves non-participating landmarks to a CSV file (matches loading)
    void ActionPromptUserToSaveNonParticipatingLandmarksToCSV(const MwDocument&, opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionPromptUserToSavePairedLandmarksToCSV(const MwDocument&, opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None);

    // prompts the user to save the mesh to an obj file
    void ActionPromptUserToSaveMeshToObjFile(const Mesh&, OBJWriterFlags);

    // prompts the user to save the mesh to an stl file
    void ActionPromptUserToMeshToStlFile(const Mesh&);

    // prompts the user to save the (already warped) points to a CSV file
    void ActionPromptUserToSaveWarpedNonParticipatingLandmarksToCSV(
        const MwDocument&,
        MwResultCache&,
        opyn::LandmarkCSVFlags = opyn::LandmarkCSVFlags::None
    );

    // swaps the source and destination (incl. premultiply, etc.)
    void ActionSwapSourceDestination(MwUndoableDocument&);

    // translate the chosen landmarks by a translation vector, but don't save the undoable output
    void ActionTranslateLandmarksDontSave(MwUndoableDocument&, const std::unordered_set<MwDocumentElementID>& landmarkIDs, const Vector3& translation);

    // commit/save the scratch space with a "translated landmarks" message (pair this with `ActionTranslateLandmarksDontSave`)
    void ActionSaveLandmarkTranslation(MwUndoableDocument&, const std::unordered_set<MwDocumentElementID>& landmarkIDs);
}
