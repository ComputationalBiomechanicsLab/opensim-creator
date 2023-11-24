#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarp/UndoableTPSDocument.hpp>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <OpenSimCreator/Utils/TPS3D.hpp>

#include <oscar/Formats/CSV.hpp>
#include <oscar/Formats/OBJ.hpp>
#include <oscar/Formats/STL.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/os.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

// user-enactable actions
namespace osc
{
    // if possible, undos the document to the last change
    void ActionUndo(UndoableTPSDocument& doc)
    {
        doc.undo();
    }

    // if possible, redos the document to the last undone change
    void ActionRedo(UndoableTPSDocument& doc)
    {
        doc.redo();
    }

    // adds a landmark to the source mesh
    void ActionAddLandmarkTo(
        UndoableTPSDocument& doc,
        TPSDocumentInputIdentifier which,
        Vec3 const& pos)
    {
        AddLandmarkToInput(doc.updScratch(), which, pos);
        doc.commitScratch("added landmark");
    }

    // prompts the user to browse for an input mesh and assigns it to the document
    void ActionBrowseForNewMesh(
        UndoableTPSDocument& doc,
        TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeMeshPath =
            PromptUserForFile(GetCommaDelimitedListOfSupportedSimTKMeshFormats());
        if (!maybeMeshPath)
        {
            return;  // user didn't select anything
        }

        UpdMesh(doc.updScratch(), which) = LoadMeshViaSimTK(*maybeMeshPath);

        doc.commitScratch("changed mesh");
    }

    // loads landmarks from a CSV file into source/destination slot of the document
    void ActionLoadLandmarksCSV(
        UndoableTPSDocument& doc,
        TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFile("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select anything
        }

        std::vector<Vec3> const landmarks = LoadLandmarksFromCSVFile(*maybeCSVPath);
        if (landmarks.empty())
        {
            return;  // the landmarks file was empty, or had invalid data
        }

        for (Vec3 const& landmark : landmarks)
        {
            AddLandmarkToInput(doc.updScratch(), which, landmark);
        }

        doc.commitScratch("loaded landmarks");
    }

    void ActionLoadNonParticipatingPointsCSV(UndoableTPSDocument& doc)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFile("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select anything
        }

        std::vector<Vec3> const landmarks = LoadLandmarksFromCSVFile(*maybeCSVPath);
        if (landmarks.empty())
        {
            return;  // the landmarks file was empty, or had invalid data
        }

        // add the new landmarks to the scratch space
        doc.updScratch().nonParticipatingLandmarks.insert(
            doc.updScratch().nonParticipatingLandmarks.end(),
            landmarks.begin(),
            landmarks.end()
        );
        doc.commitScratch("added non-participating landmarks");
    }

    // sets the TPS blending factor for the result, but does not save the change to undo/redo storage
    void ActionSetBlendFactorWithoutSaving(UndoableTPSDocument& doc, float factor)
    {
        doc.updScratch().blendingFactor = factor;
    }

    // sets the TPS blending factor for the result and saves the change to undo/redo storage
    void ActionSetBlendFactorAndSave(UndoableTPSDocument& doc, float factor)
    {
        ActionSetBlendFactorWithoutSaving(doc, factor);
        doc.commitScratch("changed blend factor");
    }

    // creates a "fresh" (default) TPS document
    void ActionCreateNewDocument(UndoableTPSDocument& doc)
    {
        doc.updScratch() = TPSDocument{};
        doc.commitScratch("created new document");
    }

    // clears all user-assigned landmarks in the TPS document
    void ActionClearAllLandmarks(UndoableTPSDocument& doc)
    {
        doc.updScratch().landmarkPairs.clear();
        doc.commitScratch("cleared all landmarks");
    }

    // clears all non-participating landmarks in the TPS document
    void ActionClearNonParticipatingLandmarks(UndoableTPSDocument& doc)
    {
        doc.updScratch().nonParticipatingLandmarks.clear();
        doc.commitScratch("cleared all non-participating landmarks");
    }

    // deletes the specified landmarks from the TPS document
    void ActionDeleteSceneElementsByID(
        UndoableTPSDocument& doc,
        std::unordered_set<TPSDocumentElementID> const& elementIDs)
    {
        if (elementIDs.empty())
        {
            return;
        }

        TPSDocument& scratch = doc.updScratch();
        for (TPSDocumentElementID const& id : elementIDs)
        {
            if (id.elementType == TPSDocumentInputElementType::Landmark)
            {
                auto const pairHasID = [&id](TPSDocumentLandmarkPair const& p) { return p.id == id.elementID; };
                auto const it = std::find_if(
                    scratch.landmarkPairs.begin(),
                    scratch.landmarkPairs.end(),
                    pairHasID
                );
                if (it != scratch.landmarkPairs.end())
                {
                    UpdLocation(*it, id.whichInput).reset();

                    if (!HasSourceOrDestinationLocation(*it))
                    {
                        // the landmark now has no data associated with it: garbage collect it
                        scratch.landmarkPairs.erase(it);
                    }
                }
            }
        }

        doc.commitScratch("deleted elements");
    }

    // saves all source/destination landmarks to a simple headerless CSV file (matches loading)
    void ActionSaveLandmarksToCSV(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream fileOutputStream{*maybeCSVPath};
        if (!fileOutputStream)
        {
            return;  // couldn't open file for writing
        }

        for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
        {
            if (std::optional<Vec3> const loc = GetLocation(p, which))
            {
                WriteCSVRow(
                    fileOutputStream,
                    std::to_array(
                    {
                        std::to_string(loc->x),
                        std::to_string(loc->y),
                        std::to_string(loc->z),
                    })
                );
            }
        }
    }

    // saves all pairable landmarks in the TPS document to a user-specified CSV file
    void ActionSaveLandmarksToPairedCSV(TPSDocument const& doc)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream fileOutputStream{*maybeCSVPath};
        if (!fileOutputStream)
        {
            return;  // couldn't open file for writing
        }

        std::vector<LandmarkPair3D> const pairs = GetLandmarkPairs(doc);

        // write header
        WriteCSVRow(
            fileOutputStream,
            std::to_array<std::string>(
            {
                "source.x",
                "source.y",
                "source.z",
                "dest.x",
                "dest.y",
                "dest.z",
            })
        );

        // write data rows
        for (LandmarkPair3D const& p : pairs)
        {
            WriteCSVRow(
                fileOutputStream,
                std::to_array(
                {
                    std::to_string(p.source.x),
                    std::to_string(p.source.y),
                    std::to_string(p.source.z),

                    std::to_string(p.destination.x),
                    std::to_string(p.destination.y),
                    std::to_string(p.destination.z),
                })
            );
        }
    }

    // prompts the user to save the mesh to an obj file
    void ActionTrySaveMeshToObj(Mesh const& mesh)
    {
        std::optional<std::filesystem::path> const maybeSavePath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("obj");
        if (!maybeSavePath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream outputFileStream
        {
            *maybeSavePath,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary
        };
        if (!outputFileStream)
        {
            return;  // couldn't open for writing
        }

        AppMetadata const& appMetadata = App::get().getMetadata();
        ObjMetadata const objMetadata
        {
            CalcFullApplicationNameWithVersionAndBuild(appMetadata),
        };

        WriteMeshAsObj(
            outputFileStream,
            mesh,
            objMetadata,
            ObjWriterFlags::NoWriteNormals  // warping might have screwed them
        );
    }

    // prompts the user to save the mesh to an stl file
    void ActionTrySaveMeshToStl(Mesh const& mesh)
    {
        std::optional<std::filesystem::path> const maybeSTLPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("stl");
        if (!maybeSTLPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream outputFileStream
        {
            *maybeSTLPath,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary,
        };
        if (!outputFileStream)
        {
            return;  // couldn't open for writing
        }

        AppMetadata const& appMetadata = App::get().getMetadata();
        StlMetadata const stlMetadata
        {
            CalcFullApplicationNameWithVersionAndBuild(appMetadata),
        };

        WriteMeshAsStl(outputFileStream, mesh, stlMetadata);
    }

    void ActionTrySaveWarpedNonParticipatingLandmarksToCSV(
        std::span<Vec3 const> nonParticipatingLandmarkPositions)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!maybeCSVPath)
        {
            return;  // user didn't select a save location
        }

        std::ofstream fileOutputStream{*maybeCSVPath};
        if (!fileOutputStream)
        {
            return;  // couldn't open file for writing
        }

        for (Vec3 const& nonParticipatingLandmark : nonParticipatingLandmarkPositions)
        {
            WriteCSVRow(
                fileOutputStream,
                std::to_array(
                {
                    std::to_string(nonParticipatingLandmark.x),
                    std::to_string(nonParticipatingLandmark.y),
                    std::to_string(nonParticipatingLandmark.z),
                })
            );
        }
    }
}

