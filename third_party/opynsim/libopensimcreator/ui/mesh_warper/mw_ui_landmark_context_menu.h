#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_helpers.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_non_participating_landmark.h>
#include <libopensimcreator/documents/mesh_warper/mw_undoable_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>

#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/utilities/string_helpers.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    class MwUiLandmarkContextMenu final : public Popup {
    public:
        explicit MwUiLandmarkContextMenu(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<MwUiSharedState> shared,
            MwDocumentElementID right_clicked_id) :

            Popup{parent, label},
            state_{std::move(shared)},
            element_id_{right_clicked_id}
        {
            set_modal(false);
        }

    private:
        void impl_draw_content() override
        {
            const MwDocumentElement* el = find_element(state_->get_scratch(), element_id_);
            if (not el) {
                request_close();  // element cannot be found in document (deleted? renamed?)
            } else if (
                const auto* landmark_pair = dynamic_cast<const MwDocumentLandmarkPair*>(el)) {

                draw_context_menu(*landmark_pair);
            } else if (
                const auto* npl = dynamic_cast<const MwDocumentNonParticipatingLandmark*>(el)) {

                draw_context_menu(*npl);
            } else {
                request_close();  // (defensive programming)
            }
        }

        void draw_context_menu(const MwDocumentLandmarkPair& lm)
        {
            // header
            draw_context_menu_header(truncate_with_ellipsis(lm.name, 15), "Landmark");
            draw_context_menu_separator();

            // name editor
            if (not active_name_edit_) {
                active_name_edit_ = lm.name;
            }
            ui::draw_string_input("name", *active_name_edit_);
            if (ui::should_save_last_drawn_item_value()) {
                action_rename_landmark(state_->upd_undoable(), lm.uid, *active_name_edit_);
                active_name_edit_.reset();
            }

            if (lm.maybe_source_location) {
                if (not active_position_edit_) {
                    active_position_edit_ = lm.maybe_source_location;
                }
                ui::draw_float3_meters_input(
                    "source           ",
                    *active_position_edit_);  // (padded to align with `destination`)
                if (ui::should_save_last_drawn_item_value()) {
                    action_set_landmark_position(
                        state_->upd_undoable(),
                        lm.uid,
                        MiDocumentInputIdentifier::Source,
                        *active_position_edit_);
                    active_position_edit_.reset();
                }
            } else {
                if (ui::draw_button("add source")) {
                    action_set_landmark_position(
                        state_->upd_undoable(),
                        lm.uid,
                        MiDocumentInputIdentifier::Source,
                        Vector3{});
                }
            }

            if (lm.maybe_destination_location) {
                if (!active_destination_position_edit_) {
                    active_destination_position_edit_ = lm.maybe_destination_location;
                }
                ui::draw_float3_meters_input("destination", *active_destination_position_edit_);
                if (ui::should_save_last_drawn_item_value()) {
                    action_set_landmark_position(
                        state_->upd_undoable(),
                        lm.uid,
                        MiDocumentInputIdentifier::Destination,
                        *active_destination_position_edit_);
                    active_position_edit_.reset();
                }
            } else {
                if (ui::draw_button_centered("add destination")) {
                    action_set_landmark_position(
                        state_->upd_undoable(),
                        lm.uid,
                        MiDocumentInputIdentifier::Destination,
                        Vector3{});
                }
            }

            draw_context_menu_separator();

            if (ui::draw_menu_item(MSMICONS_TRASH " Delete", Key::Delete)) {
                action_delete_element_by_id(state_->upd_undoable(), lm.uid);
                return;  // CARE: `lm` is now dead
            }
        }

        void draw_context_menu(const MwDocumentNonParticipatingLandmark& npl)
        {
            // header
            draw_context_menu_header(
                truncate_with_ellipsis(npl.name, 15), "Non-Participating Landmark");
            draw_context_menu_separator();

            // name editor
            if (not active_name_edit_) {
                active_name_edit_ = npl.name;
            }
            ui::draw_string_input("name", *active_name_edit_);
            if (ui::should_save_last_drawn_item_value()) {
                action_rename_non_participating_landmark(
                    state_->upd_undoable(), npl.uid, *active_name_edit_);
                active_name_edit_.reset();
            }

            if (not active_position_edit_) {
                active_position_edit_ = npl.location;
            }
            ui::draw_float3_meters_input("location", *active_position_edit_);
            if (ui::should_save_last_drawn_item_value()) {
                action_set_non_participating_landmark_position(
                    state_->upd_undoable(), npl.uid, *active_position_edit_);
                active_position_edit_.reset();
            }

            draw_context_menu_separator();

            if (ui::draw_menu_item(MSMICONS_TRASH " Delete", Key::Delete)) {
                action_delete_element_by_id(state_->upd_undoable(), npl.uid);
                return;  // CARE: `npl` is now dead
            }
        }

        std::shared_ptr<MwUiSharedState> state_;
        MwDocumentElementID element_id_;
        std::optional<std::string> active_name_edit_;
        std::optional<Vector3> active_position_edit_;
        std::optional<Vector3> active_destination_position_edit_;
    };
}
