#include "UndoableTPSDocumentActions.h"

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libopensimcreator/Documents/MeshWarper/TPSWarpResultCache.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocument.h>

#include <libopynsim/Documents/landmarks/landmark.h>
#include <libopynsim/Documents/landmarks/landmark_csv_flags.h>
#include <libopynsim/Documents/landmarks/landmark_helpers.h>
#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <liboscar/formats/csv.h>
#include <liboscar/formats/obj.h>
#include <liboscar/formats/stl.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/file_dialog_filter.h>
#include <liboscar/platform/log.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using osc::Vector3;

void osc::ActionAddLandmark(
    UndoableTPSDocument& doc,
    TPSDocumentInputIdentifier which,
    const Vector3& position)
{
    AddLandmarkToInput(doc.upd_scratch(), which, position);
    doc.commit_scratch("added landmark");
}

void osc::ActionAddNonParticipatingLandmark(
    UndoableTPSDocument& doc,
    const Vector3& position)
{
    AddNonParticipatingLandmark(doc.upd_scratch(), position);
    doc.commit_scratch("added non-participating landmark");
}

void osc::ActionSetLandmarkPosition(
    UndoableTPSDocument& doc,
    UID id,
    TPSDocumentInputIdentifier side,
    const Vector3& newPosition)
{
    TPSDocumentLandmarkPair* p = FindLandmarkPair(doc.upd_scratch(), id);
    if (!p)
    {
        return;
    }

    UpdLocation(*p, side) = newPosition;
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
    const Vector3& newPosition)
{
    auto* lm = FindNonParticipatingLandmark(doc.upd_scratch(), id);
    if (!lm)
    {
        return;
    }

    lm->location = newPosition;
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

void osc::ActionSetSourceLandmarksPrescale(UndoableTPSDocument& doc, float newSourceLandmarksPrescale)
{
    doc.upd_scratch().sourceLandmarksPrescale = newSourceLandmarksPrescale;
    doc.commit_scratch("changed source prescale factor");
}

void osc::ActionSetDestinationLandmarksPrescale(UndoableTPSDocument& doc, float newDestinationLandmarksPrescale)
{
    doc.upd_scratch().destinationLandmarksPrescale = newDestinationLandmarksPrescale;
    doc.commit_scratch("changed destination prescale factor");
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

void osc::ActionPromptUserToLoadMeshFile(
    const std::shared_ptr<UndoableTPSDocument>& doc,
    TPSDocumentInputIdentifier which)
{
    App::upd().prompt_user_to_select_file_async(
        [doc, which](const FileDialogResponse& response)
        {
            if (response.size() != 1) {
                return;  // Error or user somehow selected multiple options
            }
            try {
                const auto mesh = LoadMeshViaSimbody(response.front());
                ActionLoadMesh(*doc, mesh, which);
            }
            catch (const std::exception& ex) {
                log_error("Error importing %s: %s", response.front().c_str(), ex.what());
            }
        },
        GetSupportedSimTKMeshFormatsAsFilters()
    );
}

void osc::ActionPromptUserToLoadLandmarksFromCSV(
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
            CSV::file_dialog_filter(),
            FileDialogFilter::all_files(),
        }
    );
}

void osc::ActionPromptUserToLoadNonParticipatingLandmarksFromCSV(const std::shared_ptr<UndoableTPSDocument>& doc)
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
            CSV::file_dialog_filter(),
            FileDialogFilter::all_files(),
        }
    );
}

void osc::ActionPromptUserToSaveLandmarksToCSV(
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

        ActionWriteLandmarksAsCSV(pairs, which, flags, fout);
    }, "csv");
}

void osc::ActionWriteLandmarksAsCSV(
    std::span<const TPSDocumentLandmarkPair> pairs,
    TPSDocumentInputIdentifier which,
    lm::LandmarkCSVFlags flags,
    std::ostream& out)
{
    lm::WriteLandmarksToCSV(out, [which, it = pairs.begin(), end = pairs.end()]() mutable -> std::optional<lm::Landmark>
    {
        while (it != end) {
            const auto& pair = *it++;
            if (const auto location = GetLocation(pair, which)) {
                return lm::Landmark{std::string{pair.name}, *location};
            }
        }
        return std::nullopt;
    }, flags);
}

void osc::ActionPromptUserToSaveNonParticipatingLandmarksToCSV(
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

void osc::ActionPromptUserToSavePairedLandmarksToCSV(const TPSDocument& doc, lm::LandmarkCSVFlags flags)
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
                CSV::write_row(fout, {{"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
            }
            else {
                CSV::write_row(fout, {{"name", "source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
            }
        }

        // write data rows
        std::vector<std::string> cols;
        cols.reserve(flags & lm::LandmarkCSVFlags::NoNames ? 6 : 7);
        for (const auto& p : pairs) {
            using std::to_string;

            if (not (flags & lm::LandmarkCSVFlags::NoNames)) {
                cols.emplace_back(p.name);
            }
            cols.push_back(to_string(p.source[0]));
            cols.push_back(to_string(p.source[1]));
            cols.push_back(to_string(p.source[2]));
            cols.push_back(to_string(p.destination[0]));
            cols.push_back(to_string(p.destination[1]));
            cols.push_back(to_string(p.destination[2]));

            CSV::write_row(fout, cols);

            cols.clear();
        }
    }, "csv");
}

void osc::ActionPromptUserToSaveMeshToObjFile(const Mesh& mesh, OBJWriterFlags flags)
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

        const OBJMetadata objMetadata{
            App::get().application_name_with_version_and_buildid(),
        };

        OBJ::write(ofs, mesh, objMetadata, flags);
    }, "obj");
}

void osc::ActionPromptUserToMeshToStlFile(const Mesh& mesh)
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

        const STLMetadata stlMetadata{
            App::get().application_name_with_version_and_buildid()
        };

        STL::write(ofs, mesh, stlMetadata);
    }, "stl");
}

void osc::ActionPromptUserToSaveWarpedNonParticipatingLandmarksToCSV(
    const TPSDocument& doc,
    TPSResultCache& cache,
    lm::LandmarkCSVFlags flags)
{
    const auto span = cache.getWarpedNonParticipatingLandmarkLocations(doc);

    App::upd().prompt_user_to_save_file_with_extension_async([
        warpedNplms = std::vector<Vector3>(span.begin(), span.end()),
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

        lm::WriteLandmarksToCSV(fout, [&warpedNplms, &nplms, i = 0uz]() mutable
        {
            std::optional<lm::Landmark> rv;
            for (; !rv && i < warpedNplms.size(); ++i) {
                std::string name = std::string{nplms.at(i).name};
                rv = lm::Landmark{std::move(name), warpedNplms.at(i)};
            }
            return rv;
        }, flags);
    });
}

void osc::ActionSwapSourceDestination(UndoableTPSDocument& doc)
{
    using std::swap;

    TPSDocument& scratch = doc.upd_scratch();
    swap(scratch.destinationLandmarksPrescale, scratch.sourceLandmarksPrescale);
    swap(scratch.sourceMesh, scratch.destinationMesh);
    for (auto& lmp : scratch.landmarkPairs) {
        swap(lmp.maybeSourceLocation, lmp.maybeDestinationLocation);
    }

    doc.commit_scratch("Swapped source <--> destination");
}

void osc::ActionTranslateLandmarksDontSave(
    UndoableTPSDocument& doc,
    const std::unordered_set<TPSDocumentElementID>& landmarkIDs,
    const Vector3& translation)
{
    TPSDocument& scratch = doc.upd_scratch();
    for (const TPSDocumentElementID& id : landmarkIDs) {
        TranslateLandmarkByID(scratch, id.uid, id.input, id.type, translation);
    }
}

void osc::ActionSaveLandmarkTranslation(UndoableTPSDocument& doc, const std::unordered_set<TPSDocumentElementID>& landmarkIDs)
{
    std::stringstream ss;
    ss << "Translated " << landmarkIDs.size() << " landmarks";
    doc.commit_scratch(std::move(ss).str());
}
