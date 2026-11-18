#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_helpers.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/documents/mesh_warper/mw_result_cache.h>
#include <libopensimcreator/documents/mesh_warper/mw_undoable_document.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_hovertest_result.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_selection.h>

#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/overlay_decoration_options.h>
#include <liboscar/graphics/materials/mesh_basic_material.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/events/close_tab_event.h>
#include <liboscar/ui/popups/popup_manager.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/uid.h>

#include <concepts>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <utility>

namespace osc
{
    // top-level UI state that is shared by all UI panels
    class MwUiSharedState final {
    public:
        explicit MwUiSharedState(
            UID tab_id, Widget* parent, std::shared_ptr<SceneCache> scene_cache) :

            tab_id_{tab_id},
            parent_{parent},
            scene_cache_{std::move(scene_cache)}
        {
            OSC_ASSERT(scene_cache_ != nullptr);
            overlay_decoration_options_.set_draw_xz_grid(true);
            overlay_decoration_options_.set_draw_axis_lines(true);
            custom_rendering_options_.set_draw_floor(false);
        }

        void on_mount() { popup_manager_.on_mount(); }

        void on_unmount() {}

        void on_draw()
        {
            // draw active popups over the UI
            popup_manager_.on_draw();
        }

        const MwDocument& get_scratch() const { return undoable_tps_document_->scratch(); }

        const MwUndoableDocument& get_undoable() const { return *undoable_tps_document_; }

        MwUndoableDocument& upd_undoable() { return *undoable_tps_document_; }

        std::shared_ptr<MwUndoableDocument> get_undoable_shared_ptr()
        {
            return undoable_tps_document_;
        }

        const Mesh& get_scratch_mesh(MiDocumentInputIdentifier which) const
        {
            return get_mesh(get_scratch(), which);
        }

        const BVH& get_scratch_mesh_bvh(MiDocumentInputIdentifier which)
        {
            const Mesh& mesh = get_scratch_mesh(which);
            return upd_scene_cache().get_bvh(mesh);
        }

        MwResultCache& upd_result_cache() { return warping_cache_; }

        // returns a (potentially cached) post-TPS-warp mesh
        const Mesh& get_result_mesh()
        {
            return warping_cache_.get_warped_mesh(undoable_tps_document_->scratch());
        }

        std::span<const Vector3> get_result_non_participating_landmark_locations()
        {
            return warping_cache_.get_warped_non_participating_landmark_locations(
                undoable_tps_document_->scratch());
        }

        bool is_hovering_something() const { return current_hover_.has_value(); }

        const MwUiHovertestResult& get_current_hover() const { return *current_hover_; }

        bool is_hovered(const MwDocumentElementID& id) const
        {
            return current_hover_ && current_hover_->is_hovering(id);
        }

        void set_hover(const std::optional<MwUiHovertestResult>& new_hover)
        {
            current_hover_ = new_hover;
        }

        void set_hover(MiDocumentInputIdentifier id, const Vector3& position)
        {
            current_hover_.emplace(id, position);
        }

        void set_hover(std::nullopt_t) { current_hover_.reset(); }

        bool has_selection() const
        {
            // TODO: should probably gc the selection
            return std::ranges::any_of(user_selection_, [this](const MwDocumentElementID& el) {
                return find_element(get_scratch(), el);
            });
        }

        bool has_selection(MiDocumentInputIdentifier input) const
        {
            // TODO: should probably gc the selection
            return std::ranges::any_of(
                user_selection_, [this, input](const MwDocumentElementID& el) {
                    return el.input == input and (find_element(get_scratch(), el) != nullptr);
                });
        }

        std::vector<Vector3> get_selection_landmark_locations(MiDocumentInputIdentifier input) const
        {
            std::vector<Vector3> rv;
            for (const auto& el : user_selection_) {
                if (el.input == input) {
                    if (auto loc =
                            find_landmark_location(get_scratch(), el.uid, el.input, el.type)) {

                        rv.push_back(*loc);
                    }
                }
            }
            return rv;
        }

        std::unordered_set<MwDocumentElementID> get_selected(MiDocumentInputIdentifier input) const
        {
            std::unordered_set<MwDocumentElementID> rv;
            for (const auto& el : user_selection_) {
                if (el.input == input) {
                    rv.insert(el);
                }
            }
            return rv;
        }

        bool is_selected(const MwDocumentElementID& id) const
        {
            return user_selection_.contains(id);
        }

        void select(const MwDocumentElementID& id) { user_selection_.select(id); }

        void clear_selection() { user_selection_.clear(); }

        void select_all()
        {
            for (const auto& el : get_all_element_ids(undoable_tps_document_->scratch())) {
                user_selection_.select(el);
            }
        }

        std::unordered_set<MwDocumentElementID> get_underlying_selection_set() const
        {
            return {user_selection_.begin(), user_selection_.end()};
        }

        void close_tab()
        {
            if (parent_) {
                App::post_event<CloseTabEvent>(*parent_, tab_id_);
            }
        }

        bool can_undo() const { return undoable_tps_document_->can_undo(); }

        void undo() { undoable_tps_document_->undo(); }

        bool can_redo() const { return undoable_tps_document_->can_redo(); }

        void redo() { undoable_tps_document_->redo(); }

        template<std::derived_from<Popup> TPopup, class... Args>
        requires std::constructible_from<TPopup, Args&&...>
        void emplace_popup(Args&&... args)
        {
            popup_manager_.emplace_back<TPopup>(std::forward<Args>(args)...).open();
        }

        const Material& wireframe_material() const { return wireframe_material_; }
        const Mesh& get_landmark_sphere_mesh() const { return landmark_sphere_; }
        SceneCache& upd_scene_cache() { return *scene_cache_; }

        Vector2 get_overlay_padding() const { return {10.0f, 10.0f}; }
        Color get_paired_landmark_color() const { return Color::green(); }
        Color get_unpaired_landmark_color() const { return Color::red(); }
        Color get_non_participating_landmark_color() const { return Color::purple(); }

        const OrbitCameraController& get_linked_camera_controller() const
        {
            return linked_camera_controller_;
        }
        void set_linked_camera_controller(const OrbitCameraController& new_controller)
        {
            linked_camera_controller_ = new_controller;
        }
        bool is_cameras_linked() const { return link_cameras_; }
        void set_cameras_linked(bool v) { link_cameras_ = v; }
        bool is_only_camera_rotation_linked() const { return only_link_rotation_; }
        void set_only_camera_rotation_linked(bool v) { only_link_rotation_ = v; }
        bool update_local_camera_controller_from_linked_base(OrbitCameraController& controller)
        {
            // If the controllers are linked together, ensure the caller's is
            // updated from the linked camera this shared state holds.
            if (is_cameras_linked() and controller != linked_camera_controller_) {
                if (is_only_camera_rotation_linked()) {
                    controller.phi = linked_camera_controller_.phi;
                    controller.theta = linked_camera_controller_.theta;
                } else {
                    controller = linked_camera_controller_;
                }
                return true;
            }
            return false;
        }

        const opyn::CustomRenderingOptions& get_custom_rendering_options() const
        {
            return custom_rendering_options_;
        }
        opyn::CustomRenderingOptions& upd_custom_rendering_options()
        {
            return custom_rendering_options_;
        }
        const opyn::OverlayDecorationOptions& get_overlay_decoration_options() const
        {
            return overlay_decoration_options_;
        }
        opyn::OverlayDecorationOptions& upd_overlay_decoration_options()
        {
            return overlay_decoration_options_;
        }
        bool is_wireframe_mode_enabled() const { return wireframe_mode_; }
        void set_wireframe_mode_enabled(bool v) { wireframe_mode_ = v; }

    private:
        // ID of the top-level TPS3D tab
        UID tab_id_;

        // handle to the screen that owns the TPS3D tab
        Widget* parent_;

        // cached TPS3D algorithm result (to prevent recomputing it over and over)
        MwResultCache warping_cache_;

        // the document that the user is editing
        std::shared_ptr<MwUndoableDocument> undoable_tps_document_ =
            std::make_shared<MwUndoableDocument>();

        // `true` if the user wants the cameras to be linked
        bool link_cameras_ = true;

        // `true` if `link_cameras_` should only link the rotational parts of the cameras
        bool only_link_rotation_ = false;

        // shared linked camera
        OrbitCameraController linked_camera_controller_ = [this] {
            Camera camera;
            camera.set_vertical_field_of_view(Degrees{35.0f});
            return OrbitCameraController::focused_on(
                undoable_tps_document_->scratch().source_mesh.bounds().value_or(AABB{}), camera);
        }();

        // shared scene cache, to minimize rendering effort when redrawing
        std::shared_ptr<SceneCache> scene_cache_;

        // wireframe material, used to draw scene elements in a wireframe style
        MeshBasicMaterial wireframe_material_ = scene_cache_->wireframe_material();

        // cached sphere mesh (to prevent re-generating a sphere over and over)
        Mesh landmark_sphere_ = scene_cache_->sphere_mesh();

        // current user selection
        MwUiSelection user_selection_;

        // current user hover: reset per-frame
        std::optional<MwUiHovertestResult> current_hover_;

        // currently active tab-wide popups
        PopupManager popup_manager_;

        // user-editable rendering options
        opyn::CustomRenderingOptions custom_rendering_options_;

        // user-editable overlay decoration options
        opyn::OverlayDecorationOptions overlay_decoration_options_;

        // user-editable wireframe mode rendering toggle
        bool wireframe_mode_ = true;
    };
}
