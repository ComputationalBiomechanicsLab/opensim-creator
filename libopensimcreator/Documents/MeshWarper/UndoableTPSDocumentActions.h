#pragma once

#include <libopensimcreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocument.h>

#include <liboscar/formats/OBJ.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/utils/UID.h>

#include <iosfwd>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_set>

namespace osc { class Mesh; }
namespace osc { class TPSResultCache; }
namespace osc { struct TPSDocument; }
namespace osc { struct TPSDocumentLandmarkPair; }

// user-enactable actions
namespace osc
{
    // adds a landmark to an input mesh
    void ActionAddLandmark(UndoableTPSDocument&, TPSDocumentInputIdentifier, const Vector3&);

    // adds a non-participating landmark to the source mesh
    void ActionAddNonParticipatingLandmark(UndoableTPSDocument&, const Vector3&);

    // adds a source/destination position to an existing landmark
    void ActionSetLandmarkPosition(UndoableTPSDocument&, UID, TPSDocumentInputIdentifier, const Vector3&);

    // tries to rename the landmark to `newName`, returns the actual new name (even if it hasn't changed)
    void ActionRenameLandmark(UndoableTPSDocument&, UID, std::string_view newName);

    // sets the IDed non-participating landmark's location to the given location
    void ActionSetNonParticipatingLandmarkPosition(UndoableTPSDocument&, UID, const Vector3&);

    // tries to rename the non-particiapting landmark to `newName`, returns the actual new name (even if it hasn't changed)
    void ActionRenameNonParticipatingLandmark(UndoableTPSDocument&, UID, std::string_view newName);

    // sets the TPS blending factor for the result, but does not save the change to undo/redo storage
    void ActionSetBlendFactorWithoutCommitting(UndoableTPSDocument&, float factor);

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void ActionSetBlendFactor(UndoableTPSDocument&, float factor);

    // sets whether the engine should recalculate the mesh's normal after applying the warp
    void ActionSetRecalculatingNormals(UndoableTPSDocument&, bool newState);

    // sets the source landmark prescale for the mesh warper
    void ActionSetSourceLandmarksPrescale(UndoableTPSDocument&, float newSourceLandmarksPrescale);

    // sets the destination landmark prescale for the mesh warper
    void ActionSetDestinationLandmarksPrescale(UndoableTPSDocument&, float newDestinationLandmarksPrescale);

    // creates a "fresh" (default) TPS document
    void ActionCreateNewDocument(UndoableTPSDocument&);

    // clears all user-assigned landmarks in the TPS document
    void ActionClearAllLandmarks(UndoableTPSDocument&);

    // clears all non-participating landmarks in the TPS document
    void ActionClearAllNonParticipatingLandmarks(UndoableTPSDocument&);

    // deletes the specified landmarks from the TPS document
    void ActionDeleteSceneElementsByID(UndoableTPSDocument&, const std::unordered_set<TPSDocumentElementID>&);
    void ActionDeleteElementByID(UndoableTPSDocument&, UID);

    // assigns the given mesh to the document
    void ActionLoadMesh(UndoableTPSDocument&, const Mesh&, TPSDocumentInputIdentifier);

    // prompts the user to browse for an input mesh and assigns it to the document
    void ActionPromptUserToLoadMeshFile(const std::shared_ptr<UndoableTPSDocument>&, TPSDocumentInputIdentifier);

    // loads landmarks from a CSV file into source/destination slot of the document
    void ActionPromptUserToLoadLandmarksFromCSV(const std::shared_ptr<UndoableTPSDocument>&, TPSDocumentInputIdentifier);

    // loads non-participating landmarks from a CSV file into the source input
    void ActionPromptUserToLoadNonParticipatingLandmarksFromCSV(const std::shared_ptr<UndoableTPSDocument>&);

    // saves all source/destination landmarks to a CSV file (matches loading)
    void ActionPromptUserToSaveLandmarksToCSV(const TPSDocument&, TPSDocumentInputIdentifier, lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None);

    // writes all source/destination landmark pairs with a location to the output stream in a CSV format.
    void ActionWriteLandmarksAsCSV(std::span<const TPSDocumentLandmarkPair>, TPSDocumentInputIdentifier, lm::LandmarkCSVFlags, std::ostream&);

    // saves non-participating landmarks to a CSV file (matches loading)
    void ActionPromptUserToSaveNonParticipatingLandmarksToCSV(const TPSDocument&, lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None);

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionPromptUserToSavePairedLandmarksToCSV(const TPSDocument&, lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None);

    // prompts the user to save the mesh to an obj file
    void ActionPromptUserToSaveMeshToObjFile(const Mesh&, OBJWriterFlags);

    // prompts the user to save the mesh to an stl file
    void ActionPromptUserToMeshToStlFile(const Mesh&);

    // prompts the user to save the (already warped) points to a CSV file
    void ActionPromptUserToSaveWarpedNonParticipatingLandmarksToCSV(
        const TPSDocument&,
        TPSResultCache&,
        lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None
    );

    // swaps the source and destination (incl. premultiply, etc.)
    void ActionSwapSourceDestination(UndoableTPSDocument&);

    // translate the chosen landmarks by a translation vector, but don't save the the undoable output
    void ActionTranslateLandmarksDontSave(UndoableTPSDocument&, const std::unordered_set<TPSDocumentElementID>& landmarkIDs, const Vector3& translation);

    // commit/save the scratch space with a "translated landmarks" message (pair this with `ActionTranslateLandmarksDontSave`)
    void ActionSaveLandmarkTranslation(UndoableTPSDocument&, const std::unordered_set<TPSDocumentElementID>& landmarkIDs);
}
