#include "UndoableTPSDocumentActions.h"

#include <OpenSimCreator/Documents/Landmarks/Landmark.h>
#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/Landmarks/LandmarkHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSWarpResultCache.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocument.h>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>

#include <oscar/Formats/CSV.h>
#include <oscar/Formats/OBJ.h>
#include <oscar/Formats/STL.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

using osc::Vec3;

void osc::ActionAddLandmark(
    UndoableTPSDocument& doc,
    TPSDocumentInputIdentifier which,
    Vec3 const& pos)
{
    AddLandmarkToInput(doc.updScratch(), which, pos);
    doc.commitScratch("added landmark");
}

void osc::ActionAddNonParticipatingLandmark(
    UndoableTPSDocument& doc,
    Vec3 const& pos)
{
    AddNonParticipatingLandmark(doc.updScratch(), pos);
    doc.commitScratch("added non-participating landmark");
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

void osc::ActionDeleteElementByID(UndoableTPSDocument& doc, UID id)
{
    if (DeleteElementByID(doc.updScratch(), id))
    {
        doc.commitScratch("deleted element");
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
    auto const maybeCSVPath = PromptUserForFile("csv");
    if (!maybeCSVPath)
    {
        return;  // user didn't select anything
    }

    std::ifstream fin{*maybeCSVPath};
    if (!fin)
    {
        return;  // some kind of error opening the file
    }

    lm::ReadLandmarksFromCSV(fin, [&doc, which](auto&& landmark)
    {
        AddLandmarkToInput(doc.updScratch(), which, landmark.position, std::move(landmark.maybeName));
    });

    doc.commitScratch("loaded landmarks");
}

void osc::ActionLoadNonParticipatingLandmarksFromCSV(UndoableTPSDocument& doc)
{
    auto const maybeCSVPath = PromptUserForFile("csv");
    if (!maybeCSVPath)
    {
        return;  // user didn't select anything
    }

    std::ifstream fin{*maybeCSVPath};
    if (!fin)
    {
        return;  // some kind of error opening the file
    }

    lm::ReadLandmarksFromCSV(fin, [&doc](auto&& landmark)
    {
        AddNonParticipatingLandmark(doc.updScratch(), landmark.position, std::move(landmark.maybeName));
    });

    doc.commitScratch("added non-participating landmarks");
}

void osc::ActionSaveLandmarksToCSV(
    TPSDocument const& doc,
    TPSDocumentInputIdentifier which,
    lm::LandmarkCSVFlags flags)
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

    lm::WriteLandmarksToCSV(fout, [which, it = doc.landmarkPairs.begin(), end = doc.landmarkPairs.end()]() mutable
    {
        std::optional<lm::Landmark> rv;
        for (; !rv && it != end; ++it)
        {
            if (auto location = GetLocation(*it, which))
            {
                rv = lm::Landmark{std::string{it->name}, *location};
            }
        }
        return rv;
    }, flags);
}

void osc::ActionSavePairedLandmarksToCSV(TPSDocument const& doc, lm::LandmarkCSVFlags flags)
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

    std::vector<NamedLandmarkPair3D> const pairs = GetNamedLandmarkPairs(doc);

    // if applicable, write header row
    if (!(flags & lm::LandmarkCSVFlags::NoHeader))
    {
        if (flags & lm::LandmarkCSVFlags::NoNames)
        {
            WriteCSVRow(fout, {{"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
        }
        else
        {
            WriteCSVRow(fout, {{"name", "source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
        }
    }

    // write data rows
    std::vector<std::string> cols;
    cols.reserve(flags & lm::LandmarkCSVFlags::NoNames ? 6 : 7);
    for (auto const& p : pairs)
    {
        using std::to_string;

        if (!(flags & lm::LandmarkCSVFlags::NoNames))
        {
            cols.emplace_back(std::string{p.name});
        }
        cols.push_back(to_string(p.source.x));
        cols.push_back(to_string(p.source.y));
        cols.push_back(to_string(p.source.z));
        cols.push_back(to_string(p.destination.x));
        cols.push_back(to_string(p.destination.y));
        cols.push_back(to_string(p.destination.z));

        WriteCSVRow(fout, cols);

        cols.clear();
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
    TPSDocument const& doc,
    TPSResultCache& cache,
    lm::LandmarkCSVFlags flags)
{
    auto const maybeCSVPath = PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
    if (!maybeCSVPath)
    {
        return;  // user didn't select a save location
    }

    std::ofstream fout{*maybeCSVPath};
    if (!fout)
    {
        return;  // couldn't open file for writing
    }

    lm::WriteLandmarksToCSV(fout, [
        &doc,
        locations = cache.getWarpedNonParticipatingLandmarkLocations(doc),
        i = static_cast<size_t>(0)]() mutable
    {
        std::optional<lm::Landmark> rv;
        for (; !rv && i < locations.size(); ++i)
        {
            std::string name = std::string{doc.nonParticipatingLandmarks.at(i).name};
            rv = lm::Landmark{std::move(name), at(locations, i)};
        }
        return rv;
    }, flags);
}
