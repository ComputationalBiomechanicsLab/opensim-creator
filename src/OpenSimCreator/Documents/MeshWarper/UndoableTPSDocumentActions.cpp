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

#include <oscar/Formats/CSV.h>
#include <oscar/Formats/OBJ.h>
#include <oscar/Formats/STL.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/os.h>
#include <oscar_simbody/SimTKMeshLoader.h>

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
    const Vec3& pos)
{
    AddLandmarkToInput(doc.upd_scratch(), which, pos);
    doc.commit_scratch("added landmark");
}

void osc::ActionAddNonParticipatingLandmark(
    UndoableTPSDocument& doc,
    const Vec3& pos)
{
    AddNonParticipatingLandmark(doc.upd_scratch(), pos);
    doc.commit_scratch("added non-participating landmark");
}

void osc::ActionSetLandmarkPosition(
    UndoableTPSDocument& doc,
    UID id,
    TPSDocumentInputIdentifier side,
    const Vec3& newPos)
{
    TPSDocumentLandmarkPair* p = FindLandmarkPair(doc.upd_scratch(), id);
    if (!p)
    {
        return;
    }

    UpdLocation(*p, side) = newPos;
    doc.commit_scratch("set landmark position");
}

void osc::ActionRenameLandmark(
    UndoableTPSDocument& doc,
    UID id,
    std::string_view newName)
{
    TPSDocumentLandmarkPair* p = FindLandmarkPair(doc.upd_scratch(), id);
    if (!p)
    {
        return;
    }

    StringName name{newName};
    if (ContainsElementWithName(doc.scratch(), name))
    {
        return;  // cannot rename (already taken)
    }

    p->name = std::move(name);
    doc.commit_scratch("set landmark name");
}

void osc::ActionSetNonParticipatingLandmarkPosition(
    UndoableTPSDocument& doc,
    UID id,
    const Vec3& newPos)
{
    auto* lm = FindNonParticipatingLandmark(doc.upd_scratch(), id);
    if (!lm)
    {
        return;
    }

    lm->location = newPos;
    doc.commit_scratch("change non-participating landmark position");
}

void osc::ActionRenameNonParticipatingLandmark(
    UndoableTPSDocument& doc,
    UID id,
    std::string_view newName)
{
    auto* lm = FindNonParticipatingLandmark(doc.upd_scratch(), id);
    if (!lm)
    {
        return;  // cannot find to-be-renamed element in document
    }

    StringName name{newName};
    if (ContainsElementWithName(doc.scratch(), name))
    {
        return;  // cannot rename to new name (already taken)
    }

    lm->name = std::move(name);
    doc.commit_scratch("set non-participating landmark name");
}

void osc::ActionSetBlendFactorWithoutCommitting(UndoableTPSDocument& doc, float factor)
{
    doc.upd_scratch().blendingFactor = factor;
}

void osc::ActionSetBlendFactor(UndoableTPSDocument& doc, float factor)
{
    ActionSetBlendFactorWithoutCommitting(doc, factor);
    doc.commit_scratch("changed blend factor");
}

void osc::ActionSetRecalculatingNormals(UndoableTPSDocument& doc, bool newState)
{
    doc.upd_scratch().recalculateNormals = newState;
    const std::string_view msg = newState ?
        "enabled recalculating normals" :
        "disabled recalculating normals";
    doc.commit_scratch(msg);
}

void osc::ActionCreateNewDocument(UndoableTPSDocument& doc)
{
    doc.upd_scratch() = TPSDocument{};
    doc.commit_scratch("created new document");
}

void osc::ActionClearAllLandmarks(UndoableTPSDocument& doc)
{
    doc.upd_scratch().landmarkPairs.clear();
    doc.commit_scratch("cleared all landmarks");
}

void osc::ActionClearAllNonParticipatingLandmarks(UndoableTPSDocument& doc)
{
    doc.upd_scratch().nonParticipatingLandmarks.clear();
    doc.commit_scratch("cleared all non-participating landmarks");
}

void osc::ActionDeleteSceneElementsByID(
    UndoableTPSDocument& doc,
    const std::unordered_set<TPSDocumentElementID>& elementIDs)
{
    TPSDocument& scratch = doc.upd_scratch();
    bool somethingDeleted = false;
    for (const TPSDocumentElementID& id : elementIDs)
    {
        somethingDeleted = DeleteElementByID(scratch, id) || somethingDeleted;
    }

    if (somethingDeleted)
    {
        doc.commit_scratch("deleted elements");
    }
}

void osc::ActionDeleteElementByID(UndoableTPSDocument& doc, UID id)
{
    if (DeleteElementByID(doc.upd_scratch(), id))
    {
        doc.commit_scratch("deleted element");
    }
}

void osc::ActionLoadMesh(
    UndoableTPSDocument& doc,
    const Mesh& mesh,
    TPSDocumentInputIdentifier which)
{
    UpdMesh(doc.upd_scratch(), which) = mesh;
    doc.commit_scratch("changed mesh");
}

void osc::ActionLoadMeshFile(
    UndoableTPSDocument& doc,
    TPSDocumentInputIdentifier which)
{
    const std::optional<std::filesystem::path> maybeMeshPath =
        prompt_user_to_select_file(GetSupportedSimTKMeshFormats());
    if (not maybeMeshPath) {
        return;  // user didn't select anything
    }

    ActionLoadMesh(doc, LoadMeshViaSimTK(*maybeMeshPath), which);
}

void osc::ActionLoadLandmarksFromCSV(
    UndoableTPSDocument& doc,
    TPSDocumentInputIdentifier which)
{
    const auto maybeCSVPath = prompt_user_to_select_file({"csv"});
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
        AddLandmarkToInput(doc.upd_scratch(), which, landmark.position, std::move(landmark.maybeName));
    });

    doc.commit_scratch("loaded landmarks");
}

void osc::ActionLoadNonParticipatingLandmarksFromCSV(UndoableTPSDocument& doc)
{
    const auto maybeCSVPath = prompt_user_to_select_file({"csv"});
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
        AddNonParticipatingLandmark(doc.upd_scratch(), landmark.position, std::move(landmark.maybeName));
    });

    doc.commit_scratch("added non-participating landmarks");
}

void osc::ActionSaveLandmarksToCSV(
    const TPSDocument& doc,
    TPSDocumentInputIdentifier which,
    lm::LandmarkCSVFlags flags)
{
    const std::optional<std::filesystem::path> maybeCSVPath =
        prompt_user_for_file_save_location_add_extension_if_necessary("csv");
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

void osc::ActionSaveNonParticipatingLandmarksToCSV(
    const TPSDocument& doc,
    lm::LandmarkCSVFlags flags)
{
    const std::optional<std::filesystem::path> maybeCSVPath =
        prompt_user_for_file_save_location_add_extension_if_necessary("csv");
    if (!maybeCSVPath)
    {
        return;  // user didn't select a save location
    }

    std::ofstream fout{*maybeCSVPath};
    if (!fout)
    {
        return;  // couldn't open file for writing
    }

    lm::WriteLandmarksToCSV(fout, [it = doc.nonParticipatingLandmarks.begin(), end = doc.nonParticipatingLandmarks.end()]() mutable
    {
        std::optional<lm::Landmark> rv;
        if (it != end) {
            rv = lm::Landmark{std::string{it->name}, it->location};
        }
        ++it;
        return rv;
    }, flags);
}

void osc::ActionSavePairedLandmarksToCSV(const TPSDocument& doc, lm::LandmarkCSVFlags flags)
{
    const std::optional<std::filesystem::path> maybeCSVPath =
        prompt_user_for_file_save_location_add_extension_if_necessary("csv");
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
            write_csv_row(fout, {{"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
        }
        else
        {
            write_csv_row(fout, {{"name", "source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
        }
    }

    // write data rows
    std::vector<std::string> cols;
    cols.reserve(flags & lm::LandmarkCSVFlags::NoNames ? 6 : 7);
    for (const auto& p : pairs)
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

        write_csv_row(fout, cols);

        cols.clear();
    }
}

void osc::ActionTrySaveMeshToObjFile(const Mesh& mesh, ObjWriterFlags flags)
{
    const std::optional<std::filesystem::path> maybeSavePath =
        prompt_user_for_file_save_location_add_extension_if_necessary("obj");
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

    const AppMetadata& appMetadata = App::get().metadata();
    const ObjMetadata objMetadata
    {
        calc_full_application_name_with_version_and_build_id(appMetadata),
    };

    write_as_obj(
        outputFileStream,
        mesh,
        objMetadata,
        flags
    );
}

void osc::ActionTrySaveMeshToStlFile(const Mesh& mesh)
{
    const std::optional<std::filesystem::path> maybeSTLPath =
        prompt_user_for_file_save_location_add_extension_if_necessary("stl");
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

    const AppMetadata& appMetadata = App::get().metadata();
    const StlMetadata stlMetadata
    {
        calc_full_application_name_with_version_and_build_id(appMetadata),
    };

    write_as_stl(outputFileStream, mesh, stlMetadata);
}

void osc::ActionSaveWarpedNonParticipatingLandmarksToCSV(
    const TPSDocument& doc,
    TPSResultCache& cache,
    lm::LandmarkCSVFlags flags)
{
    const auto maybeCSVPath = prompt_user_for_file_save_location_add_extension_if_necessary("csv");
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
