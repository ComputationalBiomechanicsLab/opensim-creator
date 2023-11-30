#include "UndoableTPSDocumentActions.hpp"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocument.hpp>
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

void osc::ActionAddLandmark(
    UndoableTPSDocument& doc,
    TPSDocumentInputIdentifier which,
    Vec3 const& pos)
{
    AddLandmarkToInput(doc.updScratch(), which, pos);
    doc.commitScratch("added landmark");
}

void osc::ActionSetLandmarkPosition(
    UndoableTPSDocument& doc,
    UID id,
    TPSDocumentInputIdentifier side,
    Vec3 const& newPos)
{
    TPSDocumentLandmarkPair* p = FindLandmarkPair(doc.updScratch(), id);
    if (!p)
    {
        return;
    }

    UpdLocation(*p, side) = newPos;
    doc.commitScratch("set landmark position");
}

void osc::ActionRenameLandmark(
    UndoableTPSDocument& doc,
    UID id,
    std::string_view newName)
{
    TPSDocumentLandmarkPair* p = FindLandmarkPair(doc.updScratch(), id);
    if (!p)
    {
        return;
    }

    StringName name{newName};
    if (ContainsElementWithName(doc.getScratch(), name))
    {
        return;  // cannot rename (already taken)
    }

    p->name = std::move(name);
    doc.commitScratch("set landmark name");
}

void osc::ActionSetNonParticipatingLandmarkPosition(
    UndoableTPSDocument& doc,
    UID id,
    Vec3 const& newPos)
{
    auto* lm = FindNonParticipatingLandmark(doc.updScratch(), id);
    if (!lm)
    {
        return;
    }

    lm->location = newPos;
    doc.commitScratch("change non-participating landmark position");
}

void osc::ActionRenameNonParticipatingLandmark(
    UndoableTPSDocument& doc,
    UID id,
    std::string_view newName)
{
    auto* lm = FindNonParticipatingLandmark(doc.updScratch(), id);
    if (!lm)
    {
        return;  // cannot find to-be-renamed element in document
    }

    StringName name{newName};
    if (ContainsElementWithName(doc.getScratch(), name))
    {
        return;  // cannot rename to new name (already taken)
    }

    lm->name = std::move(name);
    doc.commitScratch("set non-participating landmark name");
}

void osc::ActionSetBlendFactorWithoutCommitting(UndoableTPSDocument& doc, float factor)
{
    doc.updScratch().blendingFactor = factor;
}

void osc::ActionSetBlendFactor(UndoableTPSDocument& doc, float factor)
{
    ActionSetBlendFactorWithoutCommitting(doc, factor);
    doc.commitScratch("changed blend factor");
}

void osc::ActionCreateNewDocument(UndoableTPSDocument& doc)
{
    doc.updScratch() = TPSDocument{};
    doc.commitScratch("created new document");
}

void osc::ActionClearAllLandmarks(UndoableTPSDocument& doc)
{
    doc.updScratch().landmarkPairs.clear();
    doc.commitScratch("cleared all landmarks");
}

void osc::ActionClearAllNonParticipatingLandmarks(UndoableTPSDocument& doc)
{
    doc.updScratch().nonParticipatingLandmarks.clear();
    doc.commitScratch("cleared all non-participating landmarks");
}

void osc::ActionDeleteSceneElementsByID(
    UndoableTPSDocument& doc,
    std::unordered_set<TPSDocumentElementID> const& elementIDs)
{
    TPSDocument& scratch = doc.updScratch();
    bool somethingDeleted = false;
    for (TPSDocumentElementID const& id : elementIDs)
    {
        somethingDeleted = DeleteElementByID(scratch, id) || somethingDeleted;
    }

    if (somethingDeleted)
    {
        doc.commitScratch("deleted elements");
    }
}

void osc::ActionLoadMeshFile(
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

void osc::ActionLoadLandmarksFromCSV(
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

void osc::ActionLoadNonParticipatingLandmarksFromCSV(UndoableTPSDocument& doc)
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

    std::transform(
        landmarks.begin(),
        landmarks.end(),
        std::back_inserter(doc.updScratch().nonParticipatingLandmarks),
        [&readonlyDoc = doc.getScratch()](Vec3 const& landmark)
        {
            return TPSDocumentNonParticipatingLandmark{NextNonParticipatingLandmarkName(readonlyDoc), landmark};
        }
    );
    doc.commitScratch("added non-participating landmarks");
}

void osc::ActionSaveLandmarksToCSV(TPSDocument const& doc, TPSDocumentInputIdentifier which, TPSDocumentCSVFlags flags)
{
    std::optional<std::filesystem::path> const maybeCSVPath =
        PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
    if (!maybeCSVPath)
    {
        return;  // user didn't select a save location
    }

    std::ofstream fout{*maybeCSVPath};
    if (!fout)
    {
        return;  // couldn't open file for writing
    }

    // if applicable, emit header
    if (!(flags & TPSDocumentCSVFlags::NoHeader))
    {
        if (flags & TPSDocumentCSVFlags::NoNames)
        {
            WriteCSVRow(fout, {{"x", "y", "z"}});
        }
        else
        {
            WriteCSVRow(fout, {{"name", "x", "y", "z"}});
        }
    }

    // emit data rows
    for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
    {
        if (std::optional<Vec3> const loc = GetLocation(p, which))
        {
            using std::to_string;
            auto x = loc->x;
            auto y = loc->y;
            auto z = loc->z;

            if (flags & TPSDocumentCSVFlags::NoNames)
            {
                WriteCSVRow(fout, {{to_string(x), to_string(y), to_string(z)}});
            }
            else
            {
                WriteCSVRow(fout, {{std::string{p.name}, to_string(x), to_string(y), to_string(z)}});
            }
        }
    }
}

void osc::ActionSavePairedLandmarksToCSV(TPSDocument const& doc)
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

void osc::ActionTrySaveMeshToObjFile(Mesh const& mesh)
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

void osc::ActionTrySaveMeshToStlFile(Mesh const& mesh)
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

void osc::ActionSaveWarpedNonParticipatingLandmarksToCSV(
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
