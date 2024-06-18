#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocument.h>

#include <oscar/Formats/OBJ.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/UID.h>

#include <string_view>
#include <unordered_set>

namespace osc { class Mesh; }
namespace osc { struct TPSDocument; }
namespace osc { class TPSResultCache; }

// user-enactable actions
namespace osc
{
    // adds a landmark to an input mesh
    void ActionAddLandmark(UndoableTPSDocument&, TPSDocumentInputIdentifier, const Vec3&);

    // adds a non-participating landmark to the source mesh
    void ActionAddNonParticipatingLandmark(UndoableTPSDocument&, const Vec3&);

    // adds a source/destination position to an existing landmark
    void ActionSetLandmarkPosition(UndoableTPSDocument&, UID, TPSDocumentInputIdentifier, const Vec3&);

    // tries to rename the landmark to `newName`, returns the actual new name (even if it hasn't changed)
    void ActionRenameLandmark(UndoableTPSDocument&, UID, std::string_view newName);

    // sets the IDed non-participating landmark's location to the given location
    void ActionSetNonParticipatingLandmarkPosition(UndoableTPSDocument&, UID, const Vec3&);

    // tries to rename the non-particiapting landmark to `newName`, returns the actual new name (even if it hasn't changed)
    void ActionRenameNonParticipatingLandmark(UndoableTPSDocument&, UID, std::string_view newName);

    // sets the TPS blending factor for the result, but does not save the change to undo/redo storage
    void ActionSetBlendFactorWithoutCommitting(UndoableTPSDocument&, float factor);

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void ActionSetBlendFactor(UndoableTPSDocument&, float factor);

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
    void ActionLoadMeshFile(UndoableTPSDocument&, TPSDocumentInputIdentifier);

    // loads landmarks from a CSV file into source/destination slot of the document
    void ActionLoadLandmarksFromCSV(UndoableTPSDocument&, TPSDocumentInputIdentifier);

    // loads non-participating landmarks from a CSV file into the source input
    void ActionLoadNonParticipatingLandmarksFromCSV(UndoableTPSDocument&);

    // saves all source/destination landmarks to a CSV file (matches loading)
    void ActionSaveLandmarksToCSV(const TPSDocument&, TPSDocumentInputIdentifier, lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None);

    // saves non-participating landmarks to a CSV file (matches loading)
    void ActionSaveNonParticipatingLandmarksToCSV(const TPSDocument&, lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None);

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionSavePairedLandmarksToCSV(const TPSDocument&, lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None);

    // prompts the user to save the mesh to an obj file
    void ActionTrySaveMeshToObjFile(const Mesh&, ObjWriterFlags);

    // prompts the user to save the mesh to an stl file
    void ActionTrySaveMeshToStlFile(const Mesh&);

    // prompts the user to save the (already warped) points to a CSV file
    void ActionSaveWarpedNonParticipatingLandmarksToCSV(
        const TPSDocument&,
        TPSResultCache&,
        lm::LandmarkCSVFlags = lm::LandmarkCSVFlags::None
    );
}
