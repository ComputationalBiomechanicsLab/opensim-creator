#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocument.hpp>

#include <oscar/Maths/Vec3.hpp>

#include <span>
#include <unordered_set>

namespace osc { class Mesh; }
namespace osc { struct TPSDocument; }

// user-enactable actions
namespace osc
{
    // adds a landmark to an input mesh
    void ActionAddLandmarkTo(UndoableTPSDocument&, TPSDocumentInputIdentifier, Vec3 const&);

    // prompts the user to browse for an input mesh and assigns it to the document
    void ActionBrowseForNewMesh(UndoableTPSDocument&, TPSDocumentInputIdentifier);

    // loads landmarks from a CSV file into source/destination slot of the document
    void ActionLoadLandmarksCSV(UndoableTPSDocument&, TPSDocumentInputIdentifier);

    // loads non-participating landmarks from a CSV file into the source input
    void ActionLoadNonParticipatingPointsCSV(UndoableTPSDocument&);

    // sets the TPS blending factor for the result, but does not save the change to undo/redo storage
    void ActionSetBlendFactorWithoutSaving(UndoableTPSDocument&, float factor);

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void ActionSetBlendFactorAndSave(UndoableTPSDocument&, float factor);

    // creates a "fresh" (default) TPS document
    void ActionCreateNewDocument(UndoableTPSDocument&);

    // clears all user-assigned landmarks in the TPS document
    void ActionClearAllLandmarks(UndoableTPSDocument&);

    // clears all non-participating landmarks in the TPS document
    void ActionClearNonParticipatingLandmarks(UndoableTPSDocument&);

    // deletes the specified landmarks from the TPS document
    void ActionDeleteSceneElementsByID(UndoableTPSDocument&, std::unordered_set<TPSDocumentElementID> const&);

    // saves all source/destination landmarks to a simple headerless CSV file (matches loading)
    void ActionSaveLandmarksToCSV(TPSDocument const&, TPSDocumentInputIdentifier);

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionSaveLandmarksToPairedCSV(TPSDocument const&);

    // prompts the user to save the mesh to an obj file
    void ActionTrySaveMeshToObj(Mesh const&);

    // prompts the user to save the mesh to an stl file
    void ActionTrySaveMeshToStl(Mesh const&);

    // prompts the user to save the (already warped) points to a CSV file
    void ActionTrySaveWarpedNonParticipatingLandmarksToCSV(std::span<Vec3 const>);
}

