#include "mw_undoable_actions.h"

#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_helpers.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>
#include <libopensimcreator/documents/mesh_warper/mw_result_cache.h>
#include <libopensimcreator/documents/mesh_warper/mw_undoable_document.h>

#include <libopynsim/documents/landmarks/landmark.h>
#include <libopynsim/documents/landmarks/landmark_csv_flags.h>
#include <libopynsim/documents/landmarks/landmark_helpers.h>
#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <liboscar/formats/csv.h>
#include <liboscar/formats/obj.h>
#include <liboscar/formats/stl.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/file_dialog_filter.h>
#include <liboscar/platform/log.h>

#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using osc::Vector3;

void osc::action_add_landmark(
    MwUndoableDocument& doc, MiDocumentInputIdentifier which, const Vector3& position)
{
    add_landmark_to_input(doc.upd_scratch(), which, position);
    doc.commit_scratch("added landmark");
}

void osc::action_add_non_participating_landmark(MwUndoableDocument& doc, const Vector3& position)
{
    add_non_participating_landmark(doc.upd_scratch(), position);
    doc.commit_scratch("added non-participating landmark");
}

void osc::action_set_landmark_position(
    MwUndoableDocument& doc, UID id, MiDocumentInputIdentifier side, const Vector3& new_position)
{
    MwDocumentLandmarkPair* p = find_landmark_pair(doc.upd_scratch(), id);
    if (!p) {
        return;
    }

    upd_location(*p, side) = new_position;
    doc.commit_scratch("set landmark position");
}

void osc::action_rename_landmark(MwUndoableDocument& doc, UID id, std::string_view new_name)
{
    MwDocumentLandmarkPair* p = find_landmark_pair(doc.upd_scratch(), id);
    if (!p) {
        return;
    }

    StringName name{new_name};
    if (contains_element_with_name(doc.scratch(), name)) {
        return;  // cannot rename (already taken)
    }

    p->name = std::move(name);
    doc.commit_scratch("set landmark name");
}

void osc::action_set_non_participating_landmark_position(
    MwUndoableDocument& doc, UID id, const Vector3& new_position)
{
    auto* lm = find_non_participating_landmark(doc.upd_scratch(), id);
    if (!lm) {
        return;
    }

    lm->location = new_position;
    doc.commit_scratch("change non-participating landmark position");
}

void osc::action_rename_non_participating_landmark(
    MwUndoableDocument& doc, UID id, std::string_view new_name)
{
    auto* lm = find_non_participating_landmark(doc.upd_scratch(), id);
    if (!lm) {
        return;  // cannot find to-be-renamed element in document
    }

    StringName name{new_name};
    if (contains_element_with_name(doc.scratch(), name)) {
        return;  // cannot rename to new name (already taken)
    }

    lm->name = std::move(name);
    doc.commit_scratch("set non-participating landmark name");
}

void osc::action_set_blend_factor_without_committing(MwUndoableDocument& doc, float factor)
{
    doc.upd_scratch().blending_factor = factor;
}

void osc::action_set_blend_factor(MwUndoableDocument& doc, float factor)
{
    action_set_blend_factor_without_committing(doc, factor);
    doc.commit_scratch("changed blend factor");
}

void osc::action_set_recalculating_normals(MwUndoableDocument& doc, bool new_state)
{
    doc.upd_scratch().recalculate_normals = new_state;
    const std::string_view msg =
        new_state ? "enabled recalculating normals" : "disabled recalculating normals";
    doc.commit_scratch(msg);
}

void osc::action_set_warping_penalty(MwUndoableDocument& doc, float new_warping_penalty)
{
    doc.upd_scratch().warping_penalty = new_warping_penalty;
    doc.commit_scratch(std::format("Set warping penalty to {}", new_warping_penalty));
}

void osc::action_set_source_landmarks_prescale(
    MwUndoableDocument& doc, float new_source_landmarks_prescale)
{
    doc.upd_scratch().source_landmarks_prescale = new_source_landmarks_prescale;
    doc.commit_scratch("changed source prescale factor");
}

void osc::action_set_destination_landmarks_prescale(
    MwUndoableDocument& doc, float new_destination_landmarks_prescale)
{
    doc.upd_scratch().destination_landmarks_prescale = new_destination_landmarks_prescale;
    doc.commit_scratch("changed destination prescale factor");
}

void osc::action_create_new_document(MwUndoableDocument& doc)
{
    doc.upd_scratch() = MwDocument{};
    doc.commit_scratch("created new document");
}

void osc::action_clear_all_landmarks(MwUndoableDocument& doc)
{
    doc.upd_scratch().landmark_pairs.clear();
    doc.commit_scratch("cleared all landmarks");
}

void osc::action_clear_all_non_participating_landmarks(MwUndoableDocument& doc)
{
    doc.upd_scratch().non_participating_landmarks.clear();
    doc.commit_scratch("cleared all non-participating landmarks");
}

void osc::action_delete_scene_elements_by_id(
    MwUndoableDocument& doc, const std::unordered_set<MwDocumentElementID>& element_ids)
{
    MwDocument& scratch = doc.upd_scratch();
    bool something_deleted = false;
    for (const MwDocumentElementID& id : element_ids) {
        something_deleted = delete_element_by_id(scratch, id) || something_deleted;
    }

    if (something_deleted) {
        doc.commit_scratch("deleted elements");
    }
}

void osc::action_delete_element_by_id(MwUndoableDocument& doc, UID id)
{
    if (delete_element_by_id(doc.upd_scratch(), id)) {
        doc.commit_scratch("deleted element");
    }
}

void osc::action_load_mesh(
    MwUndoableDocument& doc, const Mesh& mesh, MiDocumentInputIdentifier which)
{
    upd_mesh(doc.upd_scratch(), which) = mesh;
    doc.commit_scratch("changed mesh");
}

void osc::action_prompt_user_to_load_mesh_file(
    const std::shared_ptr<MwUndoableDocument>& doc, MiDocumentInputIdentifier which)
{
    App::upd().prompt_user_to_select_file_async(
        [doc, which](const FileDialogResponse& response) {
            if (response.size() != 1) {
                return;  // Error or user somehow selected multiple options
            }
            try {
                const auto mesh = opyn::load_mesh_via_simbody(response.front());
                action_load_mesh(*doc, mesh, which);
            } catch (const std::exception& ex) {
                log_error("Error importing {}: {}", response.front().string(), ex.what());
            }
        },
        opyn::get_supported_sim_tk_mesh_formats_as_filters());
}

void osc::action_prompt_user_to_load_landmarks_from_csv(
    const std::shared_ptr<MwUndoableDocument>& doc, MiDocumentInputIdentifier which)
{
    App::upd().prompt_user_to_select_file_async(
        [doc, which](const FileDialogResponse& response) {
            if (response.size() != 1) {
                return;  // Error or user somehow selected multiple files.
            }

            std::ifstream fin{response.front()};
            if (not fin) {
                return;  // some kind of error opening the file
            }

            opyn::read_landmarks_from_csv(fin, [&doc, which](auto&& landmark) {
                add_landmark_to_input(
                    doc->upd_scratch(), which, landmark.position, std::move(landmark.maybe_name));
            });

            doc->commit_scratch("loaded landmarks");
        },
        {
            CSV::file_dialog_filter(),
            FileDialogFilter::all_files(),
        });
}

void osc::action_prompt_user_to_load_non_participating_landmarks_from_csv(
    const std::shared_ptr<MwUndoableDocument>& doc)
{
    App::upd().prompt_user_to_select_file_async(
        [doc](const FileDialogResponse& response) {
            if (response.size() != 1) {
                return;  // Error or the user somehow selected more than one file.
            }

            std::ifstream fin{response.front()};
            if (not fin) {
                return;  // some kind of error opening the file
            }

            opyn::read_landmarks_from_csv(fin, [&doc](auto&& landmark) {
                add_non_participating_landmark(
                    doc->upd_scratch(), landmark.position, std::move(landmark.maybe_name));
            });

            doc->commit_scratch("added non-participating landmarks");
        },
        {
            CSV::file_dialog_filter(),
            FileDialogFilter::all_files(),
        });
}

void osc::action_prompt_user_to_save_landmarks_to_csv(
    const MwDocument& doc, MiDocumentInputIdentifier which, opyn::LandmarkCSVFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [pairs = doc.landmark_pairs, which, flags](std::optional<std::filesystem::path> p) mutable {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            std::ofstream fout{*p};
            if (not fout) {
                return;  // couldn't open file for writing
            }

            action_write_landmarks_as_csv(pairs, which, flags, fout);
        },
        "csv");
}

void osc::action_write_landmarks_as_csv(
    std::span<const MwDocumentLandmarkPair> pairs,
    MiDocumentInputIdentifier which,
    opyn::LandmarkCSVFlags flags,
    std::ostream& out)
{
    opyn::write_landmarks_to_csv(
        out,
        [which, it = pairs.begin(), end = pairs.end()] mutable -> std::optional<opyn::Landmark> {
            while (it != end) {
                const auto& pair = *it++;
                if (const auto location = get_location(pair, which)) {
                    return opyn::Landmark{std::string{pair.name}, *location};
                }
            }
            return std::nullopt;
        },
        flags);
}

void osc::action_prompt_user_to_save_non_participating_landmarks_to_csv(
    const MwDocument& doc, opyn::LandmarkCSVFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [nplms = doc.non_participating_landmarks, flags](std::optional<std::filesystem::path> p) {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            std::ofstream fout{*p};
            if (not fout) {
                return;  // couldn't open file for writing
            }

            opyn::write_landmarks_to_csv(
                fout,
                [it = nplms.begin(), end = nplms.end()] mutable {
                    std::optional<opyn::Landmark> rv;
                    if (it != end) {
                        rv = opyn::Landmark{std::string{it->name}, it->location};
                    }
                    ++it;
                    return rv;
                },
                flags);
        },
        "csv");
}

void osc::action_prompt_user_to_save_paired_landmarks_to_csv(
    const MwDocument& doc, opyn::LandmarkCSVFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [pairs = get_named_landmark_pairs(doc), flags](std::optional<std::filesystem::path> path) {
            if (not path) {
                return;  // user cancelled out of the prompt
            }
            std::ofstream fout{*path};
            if (not fout) {
                return;  // couldn't open file for writing
            }

            // if applicable, write header row
            if (not(flags & opyn::LandmarkCSVFlags::NoHeader)) {
                if (flags & opyn::LandmarkCSVFlags::NoNames) {
                    CSV::write_row(
                        fout, {{"source.x", "source.y", "source.z", "dest.x", "dest.y", "dest.z"}});
                } else {
                    CSV::write_row(
                        fout,
                        {{"name",
                          "source.x",
                          "source.y",
                          "source.z",
                          "dest.x",
                          "dest.y",
                          "dest.z"}});
                }
            }

            // write data rows
            std::vector<std::string> cols;
            cols.reserve(flags & opyn::LandmarkCSVFlags::NoNames ? 6 : 7);
            for (const auto& p : pairs) {
                using std::to_string;

                if (not(flags & opyn::LandmarkCSVFlags::NoNames)) {
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
        },
        "csv");
}

void osc::action_prompt_user_to_save_mesh_to_obj_file(const Mesh& mesh, OBJWriterFlags flags)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [mesh, flags](std::optional<std::filesystem::path> p) {
            if (not p) {
                return;  // user canceled out of the prompt
            }
            std::ofstream ofs{
                *p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
            if (not ofs) {
                return;  // couldn't open for writing
            }

            const OBJMetadata obj_metadata{
                App::get().application_name_with_version_and_buildid(),
            };

            OBJ::write(ofs, mesh, obj_metadata, flags);
        },
        "obj");
}

void osc::action_prompt_user_to_mesh_to_stl_file(const Mesh& mesh)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [mesh](std::optional<std::filesystem::path> p) {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            std::ofstream ofs{
                *p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
            if (not ofs) {
                return;  // couldn't open for writing
            }

            const STLMetadata stl_metadata{App::get().application_name_with_version_and_buildid()};

            STL::write(ofs, mesh, stl_metadata);
        },
        "stl");
}

void osc::action_prompt_user_to_save_warped_non_participating_landmarks_to_csv(
    const MwDocument& doc, MwResultCache& cache, opyn::LandmarkCSVFlags flags)
{
    const auto span = cache.get_warped_non_participating_landmark_locations(doc);

    App::upd().prompt_user_to_save_file_with_extension_async(
        [warped_nplms = std::vector<Vector3>(span.begin(), span.end()),
         nplms = doc.non_participating_landmarks,
         flags](std::optional<std::filesystem::path> p) {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            std::ofstream fout{*p};
            if (not fout) {
                return;  // couldn't open file for writing
            }

            opyn::write_landmarks_to_csv(
                fout,
                [&warped_nplms, &nplms, i = 0uz] mutable {
                    std::optional<opyn::Landmark> rv;
                    for (; !rv && i < warped_nplms.size(); ++i) {
                        std::string name = std::string{nplms.at(i).name};
                        rv = opyn::Landmark{std::move(name), warped_nplms.at(i)};
                    }
                    return rv;
                },
                flags);
        });
}

void osc::action_swap_source_destination(MwUndoableDocument& doc)
{
    using std::swap;

    MwDocument& scratch = doc.upd_scratch();
    swap(scratch.destination_landmarks_prescale, scratch.source_landmarks_prescale);
    swap(scratch.source_mesh, scratch.destination_mesh);
    for (auto& lmp : scratch.landmark_pairs) {
        swap(lmp.maybe_source_location, lmp.maybe_destination_location);
    }

    doc.commit_scratch("Swapped source <--> destination");
}

void osc::action_translate_landmarks_dont_save(
    MwUndoableDocument& doc,
    const std::unordered_set<MwDocumentElementID>& landmark_ids,
    const Vector3& translation)
{
    MwDocument& scratch = doc.upd_scratch();
    for (const MwDocumentElementID& id : landmark_ids) {
        translate_landmark_by_id(scratch, id.uid, id.input, id.type, translation);
    }
}

void osc::action_save_landmark_translation(
    MwUndoableDocument& doc, const std::unordered_set<MwDocumentElementID>& landmark_ids)
{
    std::stringstream ss;
    ss << "Translated " << landmark_ids.size() << " landmarks";
    doc.commit_scratch(std::move(ss).str());
}

void osc::action_set_landmark_locations(
    MwUndoableDocument& doc,
    const std::unordered_set<MwDocumentElementID>& landmark_ids,
    const Vector3& location)
{
    MwDocument& scratch = doc.upd_scratch();
    for (const MwDocumentElementID& id : landmark_ids) {
        set_landmark_location_by_id(scratch, id.uid, id.input, id.type, location);
    }
    doc.commit_scratch(std::format("Relocated {} landmarks", landmark_ids.size()));
}
