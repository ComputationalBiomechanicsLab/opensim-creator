#include "UndoableTPSDocumentActions.h"

#include <libopensimcreator/Documents/Landmarks/Landmark.h>
#include <libopensimcreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <libopensimcreator/Documents/Landmarks/LandmarkHelpers.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libopensimcreator/Documents/MeshWarper/TPSWarpResultCache.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocument.h>
#include <libopensimcreator/Graphics/SimTKMeshLoader.h>

#include <liboscar/Formats/CSV.h>
#include <liboscar/Formats/OBJ.h>
#include <liboscar/Formats/STL.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppMetadata.h>
#include <liboscar/Platform/os.h>

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
    const std::shared_ptr<UndoableTPSDocument>& doc,
    TPSDocumentInputIdentifier which)
{
    App::upd().prompt_user_to_select_file_async(
        [doc, which](const FileDialogResponse& response)
        {
            if (response.size() != 1) {
                return;  // Error or user somehow selected multiple options
            }
            ActionLoadMesh(*doc, LoadMeshViaSimTK(response.front()), which);
        },
        GetSupportedSimTKMeshFormatsAsFilters()
    );
}

void osc::ActionLoadLandmarksFromCSV(
    const std::shared_ptr<UndoableTPSDocument>& doc,
    TPSDocumentInputIdentifier which)
{
    App::upd().prompt_user_to_select_file_async(
        [doc, which](const FileDialogResponse& response)
        {
            if (response.size() != 1) {
                return;  // Error or user somehow selected multiple files.
            }

            std::ifstream fin{response.front()};
            if (not fin) {
                return;  // some kind of error opening the file
            }

            lm::ReadLandmarksFromCSV(fin, [&doc, which](auto&& landmark)
            {
                AddLandmarkToInput(doc->upd_scratch(), which, landmark.position, std::move(landmark.maybeName));
            });

            doc->commit_scratch("loaded landmarks");
        },
        {
            FileDialogFilter::all_files(),
            csv_file_dialog_filter(),
        }
    );
}

void osc::ActionLoadNonParticipatingLandmarksFromCSV(const std::shared_ptr<UndoableTPSDocument>& doc)
{
    App::upd().prompt_user_to_select_file_async(
        [doc](const FileDialogResponse& response)
        {
            if (response.size() != 1) {
                return;  // Error or the user somehow selected more than one file.
            }

            std::ifstream fin{response.front()};
            if (not fin) {
                return;  // some kind of error opening the file
            }

            lm::ReadLandmarksFromCSV(fin, [&doc](auto&& landmark)
            {
                AddNonParticipatingLandmark(doc->upd_scratch(), landmark.position, std::move(landmark.maybeName));
            });

            doc->commit_scratch("added non-participating landmarks");
        },
        {
            FileDialogFilter::all_files(),
            csv_file_dialog_filter(),
        }
    );
}

void osc::ActionSaveLandmarksToCSV(
    const TPSDocument& doc,
    TPSDocumentInputIdentifier which,
    lm::LandmarkCSVFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async([pairs = doc.landmarkPairs, which, flags](std::optional<std::filesystem::path> p) mutable
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }

        std::ofstream fout{*p};
        if (not fout) {
            return;  // couldn't open file for writing
        }

        lm::WriteLandmarksToCSV(fout, [which, pairs = std::move(pairs)]() mutable
        {
            std::optional<lm::Landmark> rv;
            for (const TPSDocumentLandmarkPair& pair : pairs) {
                if (const auto location = GetLocation(pair, which)) {
                    rv = lm::Landmark{std::string{pair.name}, *location};
                }
            }
            return rv;
        }, flags);
    }, "csv");
}

void osc::ActionSaveNonParticipatingLandmarksToCSV(
    const TPSDocument& doc,
    lm::LandmarkCSVFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async([nplms = doc.nonParticipatingLandmarks, flags](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }

        std::ofstream fout{*p};
        if (not fout) {
            return;  // couldn't open file for writing
        }

        lm::WriteLandmarksToCSV(fout, [it = nplms.begin(), end = nplms.end()]() mutable
        {
            std::optional<lm::Landmark> rv;
            if (it != end) {
                rv = lm::Landmark{std::string{it->name}, it->location};
            }
            ++it;
            return rv;
        }, flags);
    }, "csv");
}

void osc::ActionSavePairedLandmarksToCSV(const TPSDocument& doc, lm::LandmarkCSVFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async([pairs = GetNamedLandmarkPairs(doc), flags](std::optional<std::filesystem::path> maybePath)
    {
        if (not maybePath) {
            return;  // user cancelled out of the prompt
        }
        std::ofstream fout{*maybePath};
        if (not fout) {
            return;  // couldn't open file for writing
        }

        // if applicable, write header row
        if (not (flags & lm::LandmarkCSVFlags::NoHeader)) {
            if (flags & lm::LandmarkCSVFlags::NoNames) {
                write_csv_row(fout, {{"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
            }
            else {
                write_csv_row(fout, {{"name", "source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
            }
        }

        // write data rows
        std::vector<std::string> cols;
        cols.reserve(flags & lm::LandmarkCSVFlags::NoNames ? 6 : 7);
        for (const auto& p : pairs)
        {
            using std::to_string;

            if (not (flags & lm::LandmarkCSVFlags::NoNames)) {
                cols.emplace_back(p.name);
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
    }, "csv");
}

void osc::ActionTrySaveMeshToObjFile(const Mesh& mesh, ObjWriterFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async([mesh, flags](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }
        std::ofstream ofs{*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
        if (not ofs) {
            return;  // couldn't open for writing
        }

        const ObjMetadata objMetadata{
            App::get().application_name_with_version_and_buildid(),
        };

        write_as_obj(ofs, mesh, objMetadata, flags);
    }, "obj");
}

void osc::ActionTrySaveMeshToStlFile(const Mesh& mesh)
{
    App::upd().prompt_user_to_save_file_with_extension_async([mesh](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }

        std::ofstream ofs{*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
        if (not ofs) {
            return;  // couldn't open for writing
        }

        const StlMetadata stlMetadata{
            App::get().application_name_with_version_and_buildid()
        };

        write_as_stl(ofs, mesh, stlMetadata);
    }, "stl");
}

void osc::ActionSaveWarpedNonParticipatingLandmarksToCSV(
    const TPSDocument& doc,
    TPSResultCache& cache,
    lm::LandmarkCSVFlags flags)
{
    const auto span = cache.getWarpedNonParticipatingLandmarkLocations(doc);

    App::upd().prompt_user_to_save_file_with_extension_async([
        warpedNplms = std::vector<Vec3>(span.begin(), span.end()),
        nplms = doc.nonParticipatingLandmarks,
        flags](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }

        std::ofstream fout{*p};
        if (not fout) {
            return;  // couldn't open file for writing
        }

        lm::WriteLandmarksToCSV(fout, [&warpedNplms, &nplms, i = static_cast<size_t>(0)]() mutable
        {
            std::optional<lm::Landmark> rv;
            for (; !rv && i < warpedNplms.size(); ++i)
            {
                std::string name = std::string{nplms.at(i).name};
                rv = lm::Landmark{std::move(name), warpedNplms.at(i)};
            }
            return rv;
        }, flags);
    });
}
