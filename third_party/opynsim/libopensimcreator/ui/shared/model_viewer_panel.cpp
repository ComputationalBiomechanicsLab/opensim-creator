#include "model_viewer_panel.h"

#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/model_selection_gizmo.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_layer.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_right_click_event.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_state.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/model/model_state_pair_info.h>
#include <libopynsim/graphics/cached_model_renderer.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/ui/widgets/gui_ruler.h>
#include <liboscar/ui/widgets/icon_without_menu.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    std::string get_settings_key_prefix_for_panel(std::string_view panel_name)
    {
        std::stringstream ss;
        ss << "panels/" << panel_name << '/';
        return std::move(ss).str();
    }

    class RulerLayer final : public ModelViewerPanelLayer {
    public:
        RulerLayer() { ruler_.start_measuring(); }

    private:
        ModelViewerPanelLayerFlags impl_get_flags() const override
        {
            return ModelViewerPanelLayerFlags::CapturesMouseInputs;
        }

        bool impl_handle_mouse_inputs(ModelViewerPanelParameters&, ModelViewerPanelState&) override
        {
            return true;  // always handles the mouse
        }

        void impl_on_draw(ModelViewerPanelParameters& params, ModelViewerPanelState& state) override
        {
            ruler_.on_draw(
                params.get_camera(), state.viewport_ui_rect, state.maybe_base_layer_hittest);
        }

        bool impl_should_close() const override { return !ruler_.is_measuring(); }

        GuiRuler ruler_;
    };

    // model viewer layer that adds buttons for controling visualization
    // options and 3D manipulator gizmos
    class ButtonAndGizmoControlsLayer final : public ModelViewerPanelLayer {
    public:
        ButtonAndGizmoControlsLayer(
            std::string_view panel_name, std::shared_ptr<opyn::ModelStatePair> model) :

            panel_name_{panel_name},
            gizmo_{std::move(model)}
        {}

    private:
        ModelViewerPanelLayerFlags impl_get_flags() const override
        {
            ModelViewerPanelLayerFlags flags = ModelViewerPanelLayerFlags::None;
            if (gizmo_.is_using()) {
                flags |= ModelViewerPanelLayerFlags::CapturesMouseInputs;
            }
            return flags;
        }

        float impl_get_background_alpha() const override { return 0.0f; }

        bool impl_handle_mouse_inputs(ModelViewerPanelParameters&, ModelViewerPanelState&) override
        {
            // care: `is_using` can return `true` even if it isn't being drawn this frame
            return gizmo_.is_using();
        }

        bool impl_handle_keyboard_inputs(
            ModelViewerPanelParameters&, ModelViewerPanelState&) override
        {
            return gizmo_.handle_keyboard_inputs();
        }

        void impl_on_draw(ModelViewerPanelParameters& params, ModelViewerPanelState& state) final
        {
            // draw generic overlays (i.e. the buttons for toggling things)
            auto render_params_before = params.get_render_params();

            const bool edited = draw_viewer_im_gui_overlays(
                params.upd_render_params(),
                params.upd_orbit_camera_controller(),
                state.get_drawlist(),
                state.maybe_scene_visible_aabb,
                state.viewport_ui_rect,
                *icon_cache_,
                [this, &state] { return draw_extra_top_buttons(state); });

            if (edited) {
                log_debug("{} edited", panel_name_);

                const auto& render_params_after = params.get_render_params();

                save_model_renderer_params_difference(
                    render_params_before,
                    render_params_after,
                    get_settings_key_prefix_for_panel(panel_name_),
                    App::upd_settings());
            }

            // draw gizmo manipulators over the top
            gizmo_.on_draw(state.viewport_ui_rect, params.get_camera());
        }

        bool impl_should_close() const override
        {
            return false;  // never closes
        }

        // draws extra top overlay buttons
        bool draw_extra_top_buttons(ModelViewerPanelState& state)
        {
            bool edited = false;

            IconWithoutMenu ruler_button{
                icon_cache_->find_or_throw("ruler"),
                "Ruler",
                "Roughly measure something in the scene",
            };
            if (ruler_button.on_draw()) {
                state.push_layer(std::make_unique<RulerLayer>());
                edited = true;
            }

            ui::same_line();
            ui::draw_vertical_separator();
            ui::same_line();

            // draw translate/rotate/scale selector
            if (ui::draw_gizmo_operation_selector(
                    gizmo_,
                    true,
                    true,
                    false,
                    MSMICONS_ARROWS_ALT,
                    MSMICONS_REDO,
                    MSMICONS_EXPAND_ARROWS_ALT)) {
                edited = true;
            }

            ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
            ui::same_line();
            ui::pop_style_var();

            // draw global/world selector
            if (ui::draw_gizmo_mode_selector(gizmo_)) {
                edited = true;
            }

            return edited;
        }

        std::shared_ptr<IconCache> icon_cache_ = App::singleton<IconCache>(
            App::resource_loader().with_prefix("OpenSimCreator/icons/"),
            ui::get_font_base_size() / 128.0f,
            App::get().highest_device_pixel_ratio());
        std::string panel_name_;
        ModelSelectionGizmo gizmo_;
    };

    // `ModelViewerPanelLayer` that overlays non-interactive 2D annotations
    // over the 3D render.
    class InformationalOverlaysLayer final : public ModelViewerPanelLayer {
    public:
        explicit InformationalOverlaysLayer() = default;

    private:
        ModelViewerPanelLayerFlags impl_get_flags() const override
        {
            return ModelViewerPanelLayerFlags::None;
        }

        void impl_on_draw(ModelViewerPanelParameters& params, ModelViewerPanelState& state) override
        {
            draw_coordinate_line_overlay_or_reset_cache(params, state);
        }

        void draw_coordinate_line_overlay_or_reset_cache(
            ModelViewerPanelParameters& params, ModelViewerPanelState& state)
        {
            // Update cached `OpenSim::Joint` transform samples with respect to the
            // selected `OpenSim::Coordinate`, if applicable.
            auto& cache = cached_coordinate_overlay_state_;
            if (opyn::ModelStatePairInfo current_model_state_pair{*params.get_model_shared_ptr()};
                current_model_state_pair != previous_model_state_pair_) {

                // Cache key has changed: clear/reset cached stuff.
                cache.clear();

                if (const auto* coordinate =
                        params.get_model_shared_ptr()->get_selected_as<OpenSim::Coordinate>();
                    (coordinate != nullptr) and
                    not coordinate->getLocked(params.get_model_shared_ptr()->get_state())) {

                    // If the caller has an `OpenSim::Coordinate` selected, and it isn't
                    // locked, sample `[min, max]` to figure out how the joint transform
                    // changes with respect to the coordinate.
                    try {
                        const auto& associated_joint = coordinate->getJoint();
                        const auto& associated_child_frame = associated_joint.getChildFrame();
                        const ClosedInterval<double> coordinate_range{
                            coordinate->getRangeMin(), coordinate->getRangeMax()};
                        const double sampler_step_size =
                            coordinate_range.step_size(c_num_coordinate_sample_points);

                        // Save current joint transform (used for showing user current
                        // state-of-play).
                        SimTK::State sampling_state{params.get_model_shared_ptr()->get_state()};
                        params.get_model_shared_ptr()->get_model().realizePosition(sampling_state);
                        cache.currentTransform =
                            associated_child_frame.getTransformInGround(sampling_state);

                        // Sample along the coordinate, caching transforms.
                        cache.sampledTransforms.reserve(c_num_coordinate_sample_points);
                        for (size_t step = 0; step < c_num_coordinate_sample_points; ++step) {
                            const double sampled_coordinate_value =
                                coordinate_range.lower +
                                static_cast<double>(step) * sampler_step_size;
                            coordinate->setValue(sampling_state, sampled_coordinate_value, false);
                            params.get_model_shared_ptr()->get_model().realizePosition(
                                sampling_state);
                            cache.sampledTransforms.push_back(
                                associated_child_frame.getTransformInGround(sampling_state));
                        }
                    } catch (const std::exception& ex) {
                        // There was a problem sampling the points.
                        //
                        // Because this is a visual overlay, we recover by just not showing the
                        // overlay.
                        cache.clear();
                        log_warn(
                            "ModelViewerPanel: error sampling selected coordinate {}: {}",
                            coordinate->getName(),
                            ex.what());
                    }
                }
                previous_model_state_pair_ =
                    std::move(current_model_state_pair);  // Update cache key
            }

            // If the user (still) has an `OpenSim::Coordinate` selected, and the cache is
            // populated with enough data, draw the overlay.
            if (const auto* coordinate =
                    params.get_model_shared_ptr()->get_selected_as<OpenSim::Coordinate>();
                coordinate != nullptr and cache.sampledTransforms.size() >= 2) {

                // Represents points projected from a transform into ui space.
                struct ProjectedPoints {
                    ProjectedPoints() = default;

                    explicit ProjectedPoints(
                        const Camera& camera,
                        float focal_distance,
                        const Rect& viewport_ui_rect,
                        const SimTK::Transform& t)
                    {
                        const float viewport_fill_percentage =
                            c_frame_leg_projection_in_screen_space / viewport_ui_rect.height();
                        const float scale = viewport_fill_percentage *
                                            camera.view_volume_height_at_depth(focal_distance);

                        const auto world_origin =
                            to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(0.0, 0.0, 0.0)));
                        const auto world_x =
                            to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(scale, 0.0, 0.0)));
                        const auto world_y =
                            to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(0.0, scale, 0.0)));
                        const auto world_z =
                            to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(0.0, 0.0, scale)));

                        origin = camera.world_to_ui(world_origin, viewport_ui_rect);
                        x = camera.world_to_ui(world_x, viewport_ui_rect);
                        y = camera.world_to_ui(world_y, viewport_ui_rect);
                        z = camera.world_to_ui(world_z, viewport_ui_rect);
                    }

                    Vector2 origin, x, y, z;
                };

                auto dl = ui::get_panel_draw_list();
                const auto& viewport_ui_rect = state.viewport_ui_rect;
                const auto& camera = params.get_camera();
                const float focal_distance = params.get_orbit_camera_controller().radius;

                // Draw lines along the sample points, so users can see
                // the "rail" of the coordinate.
                {
                    ProjectedPoints previous{
                        camera, focal_distance, viewport_ui_rect, cache.sampledTransforms[0]};
                    for (size_t i = 1; i < cache.sampledTransforms.size(); ++i) {
                        ProjectedPoints current{
                            camera, focal_distance, viewport_ui_rect, cache.sampledTransforms[i]};

                        dl.add_line(
                            previous.x,
                            current.x,
                            Color::red().with_alpha(c_coordinate_axis_alpha),
                            c_overlay_thickness);
                        dl.add_line(
                            previous.y,
                            current.y,
                            Color::green().with_alpha(c_coordinate_axis_alpha),
                            c_overlay_thickness);
                        dl.add_line(
                            previous.z,
                            current.z,
                            Color::blue().with_alpha(c_coordinate_axis_alpha),
                            c_overlay_thickness);

                        previous = current;
                    }
                }

                // If the `OpenSim::Coordinate` is clamped, put an endcap on the rail, so that
                // users can see that a coordinate must stop at the ends.
                if (coordinate->getClamped(params.get_model_shared_ptr()->get_state())) {

                    const auto draw_caps = [](ui::DrawListView& dl,
                                              const ProjectedPoints& a,
                                              const ProjectedPoints& b) {
                        const auto draw_cap = [](ui::DrawListView& dl,
                                                 const Vector2& p1,
                                                 const Vector2& p2,
                                                 const Color& color,
                                                 float offset) {
                            const Vector2 delta = p2 - p1;
                            const float delta_length = length(delta);
                            if (delta_length < 0.0001f) {
                                // Don't draw a cap if the last two points are basically on top of
                                // each other
                                return;
                            }
                            const Vector2 line_direction = delta / delta_length;
                            const Vector2 endpoint_with_offset = p2 + offset * line_direction;
                            const Vector2 endcap_norm{-line_direction.y(), line_direction.x()};
                            const Vector2 endcap_start =
                                endpoint_with_offset - (2.0f * c_overlay_thickness * endcap_norm);
                            const Vector2 endcap_end =
                                endpoint_with_offset + (2.0f * c_overlay_thickness * endcap_norm);

                            dl.add_line(endcap_start, endcap_end, color, c_overlay_thickness);
                        };
                        draw_cap(
                            dl,
                            b.origin,
                            a.origin,
                            Color::white().with_alpha(c_coordinate_axis_alpha),
                            0.5f * c_overlay_thickness);
                        draw_cap(
                            dl,
                            b.x,
                            a.x,
                            Color::red().with_alpha(c_coordinate_axis_alpha),
                            0.5f * c_overlay_thickness);
                        draw_cap(
                            dl,
                            b.y,
                            a.y,
                            Color::green().with_alpha(c_coordinate_axis_alpha),
                            0.5f * c_overlay_thickness);
                        draw_cap(
                            dl,
                            b.z,
                            a.z,
                            Color::blue().with_alpha(c_coordinate_axis_alpha),
                            0.5f * c_overlay_thickness);
                    };

                    // Min caps
                    draw_caps(
                        dl,
                        ProjectedPoints{
                            camera, focal_distance, viewport_ui_rect, cache.sampledTransforms[0]},
                        ProjectedPoints{
                            camera, focal_distance, viewport_ui_rect, cache.sampledTransforms[1]});

                    // Max caps
                    draw_caps(
                        dl,
                        ProjectedPoints(
                            camera,
                            focal_distance,
                            viewport_ui_rect,
                            cache.sampledTransforms.back()),
                        ProjectedPoints(
                            camera,
                            focal_distance,
                            viewport_ui_rect,
                            *(cache.sampledTransforms.rbegin() + 1)));
                }

                // Draw a frame-like core representing the coordinate's current state.
                {
                    const ProjectedPoints pp{
                        camera, focal_distance, viewport_ui_rect, cache.currentTransform};

                    // Legs
                    dl.add_line(pp.origin, pp.x, Color::red(), c_overlay_thickness);
                    dl.add_line(pp.origin, pp.y, Color::green(), c_overlay_thickness);
                    dl.add_line(pp.origin, pp.z, Color::blue(), c_overlay_thickness);

                    // Circles
                    dl.add_circle_filled({.origin = pp.x, .radius = c_core_radius}, Color::red());
                    dl.add_circle_filled({.origin = pp.y, .radius = c_core_radius}, Color::green());
                    dl.add_circle_filled({.origin = pp.z, .radius = c_core_radius}, Color::blue());
                    dl.add_circle_filled(
                        {.origin = pp.origin, .radius = c_core_radius}, Color::white());
                }
            }
        }

        opyn::ModelStatePairInfo previous_model_state_pair_;
        struct CachedCoordinateOverlayState {
            void clear() { sampledTransforms.clear(); }

            SimTK::Transform currentTransform;
            std::vector<SimTK::Transform> sampledTransforms;
        };
        CachedCoordinateOverlayState cached_coordinate_overlay_state_;
        static constexpr size_t c_num_coordinate_sample_points = 100;
        static constexpr float c_frame_leg_projection_in_screen_space = 128.0f;
        static constexpr float c_overlay_thickness = 5.0f;
        static constexpr float c_core_radius = 1.25f * c_overlay_thickness;
        static constexpr float c_coordinate_axis_alpha = 0.45f;
    };

    // the "base" model viewer layer, which is the last layer to handle any input
    // etc. if no upper layer decides to handle it
    class BaseInteractionLayer final : public ModelViewerPanelLayer {
    private:
        void impl_on_new_frame() override { is_handling_mouse_inputs_ = false; }

        bool impl_handle_keyboard_inputs(
            ModelViewerPanelParameters& params, ModelViewerPanelState& state) override
        {
            const bool controller_updated = ui::update_orbit_controller_from_keyboard_inputs(
                params.upd_orbit_camera_controller(),
                params.get_camera(),
                state.viewport_ui_rect,
                state.maybe_scene_visible_aabb);
            if (controller_updated) {
                params.get_orbit_camera_controller().update_camera(params.upd_camera());
            }
            return controller_updated;
        }

        bool impl_handle_mouse_inputs(
            ModelViewerPanelParameters& params, ModelViewerPanelState& state) override
        {
            is_handling_mouse_inputs_ = true;

            // try updating the camera (mouse panning, etc.)
            bool rv = ui::update_orbit_controller_from_mouse_inputs(
                params.upd_orbit_camera_controller(),
                params.get_camera(),
                state.viewport_ui_rect.dimensions());
            if (rv) {
                params.get_orbit_camera_controller().update_camera(params.upd_camera());
            }

            if (ui::is_mouse_dragging_with_any_button_down()) {
                params.get_model_shared_ptr()->set_hovered(nullptr);
            } else if (
                state.maybe_hovered_component_abs_path !=
                opyn::get_absolute_path_or_empty(params.get_model_shared_ptr()->get_hovered())) {

                // care: this code must check whether the hover != current hover
                // (even if null), because there might be multiple viewports open
                // (#582)
                params.get_model_shared_ptr()->set_hovered(
                    opyn::find_component(
                        params.get_model_shared_ptr()->get_model(),
                        state.maybe_hovered_component_abs_path));
                rv = true;
            }

            // if left-clicked, update top-level model selection
            if (state.is_left_click_released_without_dragging) {
                params.get_model_shared_ptr()->set_selected(
                    opyn::find_component(
                        params.get_model_shared_ptr()->get_model(),
                        state.maybe_hovered_component_abs_path));
                rv = true;
            }

            return rv;
        }

        void impl_on_draw(ModelViewerPanelParameters& params, ModelViewerPanelState& state) override
        {
            // hover, but not panning: show tooltip
            if (not state.maybe_hovered_component_abs_path.toString().empty() and
                is_handling_mouse_inputs_ and not ui::is_mouse_dragging_with_any_button_down()) {

                if (const OpenSim::Component* c = opyn::find_component(
                        params.get_model_shared_ptr()->get_model(),
                        state.maybe_hovered_component_abs_path)) {
                    draw_component_hover_tooltip(*c);
                }
            }

            // right-click: open context menu
            if (is_handling_mouse_inputs_ and state.is_right_click_released_without_dragging) {

                // right-click: pump a right-click event
                const ModelViewerPanelRightClickEvent e{
                    std::string{state.get_panel_name()},
                    state.viewport_ui_rect,
                    state.maybe_hovered_component_abs_path.toString(),
                    state.maybe_base_layer_hittest
                        ? std::optional<Vector3>{state.maybe_base_layer_hittest->world_position}
                        : std::nullopt,
                };
                params.call_on_right_click_handler(e);
            }
        }

        bool impl_should_close() const override { return false; }

        bool is_handling_mouse_inputs_ = false;
    };
}

class osc::ModelViewerPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        ModelViewerPanel& owner,
        Widget* parent,
        std::string_view panel_name,
        ModelViewerPanelParameters parameters,
        ModelViewerPanelFlags flags) :

        PanelPrivate{owner, parent, panel_name},
        parameters_{std::move(parameters)},
        state_{name(), flags}
    {
        // update this panel's rendering/state parameters from the runtime
        // configuration (e.g. user edits)
        //
        // each panel has its own configuration set (`panels/viewer0,1,2, etc.`)
        upd_model_renderer_params_from(
            App::settings(),
            get_settings_key_prefix_for_panel(panel_name),
            parameters_.upd_render_params());
        push_layer(std::make_unique<BaseInteractionLayer>());
        push_layer(std::make_unique<InformationalOverlaysLayer>());
        push_layer(
            std::make_unique<ButtonAndGizmoControlsLayer>(
                panel_name, parameters_.get_model_shared_ptr()));
    }

    bool is_moused_over() const { return render_is_hovered_; }

    bool is_left_clicked() const
    {
        return render_is_hovered_ and state_.is_left_click_released_without_dragging;
    }

    bool is_right_clicked() const
    {
        return render_is_hovered_ and state_.is_right_click_released_without_dragging;
    }

    ModelViewerPanelLayer& push_layer(std::unique_ptr<ModelViewerPanelLayer> layer)
    {
        // care: do not push new layers directly into `m_Layers`, because `pushLayer` can be
        // called during iteration over `m_Layers` (e.g. during drawing)
        return state_.push_layer(std::move(layer));
    }

    void focus_on(const Vector3& position)
    {
        parameters_.upd_orbit_camera_controller().focus_point = position;
        parameters_.get_orbit_camera_controller().update_camera(parameters_.upd_camera());
    }

    std::optional<Rect> get_screen_rect() const { return state_.viewport_ui_rect; }

    const Camera& get_camera() const { return parameters_.get_camera(); }
    Camera& upd_camera() { return parameters_.upd_camera(); }

    const OrbitCameraController& get_orbit_camera_controller() const
    {
        return parameters_.get_orbit_camera_controller();
    }
    OrbitCameraController& upd_orbit_camera_controller()
    {
        return parameters_.upd_orbit_camera_controller();
    }

    void set_model_state(const std::shared_ptr<opyn::ModelStatePair>& new_model_state)
    {
        parameters_.set_model_shared_ptr(new_model_state);
    }

    void draw_content()
    {
        // HACK: garbage-collect one frame later, because the layers
        // may have submitted textures to ImGui that are then invalid
        // because GCing destroyed them before they were rendered
        layers_garbage_collect();

        state_.viewport_ui_rect = ui::get_content_region_available_ui_rect();
        state_.is_left_click_released_without_dragging =
            ui::is_mouse_released_without_dragging(MouseButton::Left);
        state_.is_right_click_released_without_dragging =
            ui::is_mouse_released_without_dragging(MouseButton::Right);

        // if necessary, auto-focus the camera on the first frame
        if (is_first_frame_) {
            const auto bounds = state_.upd_renderer().visible_bounds(
                *parameters_.get_model_shared_ptr(), parameters_.get_render_params());
            if (bounds) {
                parameters_.upd_orbit_camera_controller().focus_on(
                    *bounds, parameters_.get_camera(), aspect_ratio_of(state_.viewport_ui_rect));
                parameters_.get_orbit_camera_controller().update_camera(parameters_.upd_camera());
            }
            is_first_frame_ = false;
        }

        layers_on_new_frame();

        // if the viewer is hovered, handle inputs
        if (render_is_hovered_) {
            layers_handle_mouse_inputs();

            if (not ui::wants_keyboard()) {
                layers_handle_keyboard_inputs();
            }
        }

        // render the 3D scene to a texture and present it via a ui::Image
        {
            RenderTexture& scene_texture = state_.upd_renderer().on_draw(
                *parameters_.get_model_shared_ptr(),
                parameters_.get_render_params(),
                state_.viewport_ui_rect.dimensions(),
                App::settings().get_value<float>("graphics/render_scale", 1.0f) *
                    App::get().main_window_device_pixel_ratio(),
                App::get().anti_aliasing_level());
            ui::draw_image(scene_texture, state_.viewport_ui_rect.dimensions());

            // care: hittesting is done here, rather than using ui::is_panel_hovered, because
            // we care about whether the _render_ is hovered, not any part of the window (which
            // may include things like the title bar, etc.
            //
            // screwing this up can result in unusual camera behavior, e.g. the camera may move when
            // dragging a visualizer panel around (#739 #93)

            // check if the window is conditionally hovered: this returns true if no other window is
            // overlapping the editor panel, _but_ it also returns true if the user is only hovering
            // the title bar of the window, rather than specifically the render
            const bool window_hovered = ui::is_panel_hovered(ui::HoveredFlag::ChildPanels);

            // check if the 3D render is hovered - ignore blocking and overlapping because the layer
            // stack might be screwing with this
            const bool render_hovered_ignoring_overlap = ui::is_item_hovered({
                ui::HoveredFlag::AllowWhenBlockedByActiveItem,
                ui::HoveredFlag::AllowWhenOverlapped,
            });

            render_is_hovered_ = window_hovered && render_hovered_ignoring_overlap;
        }

        // update state scene AABB
        state_.maybe_scene_visible_aabb = state_.get_renderer().visible_bounds();

        // if hovering in 2D, 3D-hittest the scene
        if (render_is_hovered_ and not(state_.flags() & ModelViewerPanelFlag::NoHittest)) {
            state_.maybe_base_layer_hittest = state_.get_renderer().get_closest_collision(
                parameters_.get_render_params(),
                ui::get_mouse_ui_position(),
                state_.viewport_ui_rect);
        } else {
            state_.maybe_base_layer_hittest.reset();
        }

        // if there's a 3D-hit, transform it into an OpenSim-hit
        if (state_.maybe_base_layer_hittest) {
            state_.maybe_hovered_component_abs_path =
                OpenSim::ComponentPath{std::string{state_.maybe_base_layer_hittest->decoration_id}};
        } else {
            state_.maybe_hovered_component_abs_path = {};
        }

        layers_draw();
        layers_pop_queued_new_layers();
    }

private:
    void layers_on_new_frame()
    {
        for (const auto& layer_ptr : layers_) {
            layer_ptr->on_new_frame();
        }
    }

    void layers_handle_keyboard_inputs()
    {
        for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
            if ((*it)->handle_keyboard_inputs(parameters_, state_)) {
                return;
            }
        }
    }

    void layers_handle_mouse_inputs()
    {
        for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
            if ((*it)->handle_mouse_inputs(parameters_, state_) ||
                (*it)->get_flags() & ModelViewerPanelLayerFlags::CapturesMouseInputs) {
                return;
            }
        }
    }

    void layers_draw()
    {
        for (auto it = layers_.begin(); it != layers_.end(); ++it) {
            ModelViewerPanelLayer& layer = **it;

            ui::PanelFlags window_flags =
                ui::get_minimal_panel_flags().without(ui::PanelFlag::NoInputs);

            // if any layer above this one captures mouse inputs then disable this layer's inputs
            if (find_if(it + 1, layers_.end(), [](const auto& layer_ptr) -> bool {
                    return layer_ptr->get_flags() & ModelViewerPanelLayerFlags::CapturesMouseInputs;
                }) != layers_.end()) {

                window_flags |= ui::PanelFlag::NoInputs;
            }

            // layers always have a background (although, it can be entirely invisible)
            window_flags = window_flags.without(ui::PanelFlag::NoBackground);
            ui::set_next_panel_bg_alpha(layer.get_background_alpha());

            // draw the layer in a child window, so that ImGui understands that hittests
            // should happen window-by-window (otherwise, you'll have problems with overlapping
            // buttons, widgets, etc.)
            ui::set_next_panel_ui_position(state_.viewport_ui_rect.ypd_top_left());
            const std::string child_id = std::to_string(std::distance(it, layers_.end()));
            if (ui::begin_child_panel(
                    child_id,
                    state_.viewport_ui_rect.dimensions(),
                    ui::ChildPanelFlags{},
                    window_flags)) {
                layer.on_draw(parameters_, state_);
                ui::end_child_panel();
            }
        }
    }

    void layers_garbage_collect()
    {
        std::erase_if(layers_, [](const auto& layer_ptr) { return layer_ptr->should_close(); });
    }

    void layers_pop_queued_new_layers() { state_.flush_layer_queue_to(layers_); }

    ModelViewerPanelParameters parameters_;
    ModelViewerPanelState state_;
    std::vector<std::unique_ptr<ModelViewerPanelLayer>> layers_;
    bool is_first_frame_ = true;
    bool render_is_hovered_ = false;
};

osc::ModelViewerPanel::ModelViewerPanel(
    Widget* parent,
    std::string_view panel_name,
    const ModelViewerPanelParameters& parameters,
    ModelViewerPanelFlags flags) :

    Panel{std::make_unique<Impl>(*this, parent, panel_name, parameters, flags)}
{}
bool osc::ModelViewerPanel::is_moused_over() const
{
    return private_data().is_moused_over();
}
bool osc::ModelViewerPanel::is_left_clicked() const
{
    return private_data().is_left_clicked();
}
bool osc::ModelViewerPanel::is_right_clicked() const
{
    return private_data().is_right_clicked();
}
ModelViewerPanelLayer& osc::ModelViewerPanel::push_layer(
    std::unique_ptr<ModelViewerPanelLayer> layer)
{
    return private_data().push_layer(std::move(layer));
}
void osc::ModelViewerPanel::focus_on(const Vector3& position)
{
    private_data().focus_on(position);
}
std::optional<Rect> osc::ModelViewerPanel::get_screen_rect() const
{
    return private_data().get_screen_rect();
}
const Camera& osc::ModelViewerPanel::get_camera() const
{
    return private_data().get_camera();
}
Camera& osc::ModelViewerPanel::upd_camera()
{
    return private_data().upd_camera();
}
const OrbitCameraController& osc::ModelViewerPanel::get_orbit_camera_controller() const
{
    return private_data().get_orbit_camera_controller();
}
OrbitCameraController& osc::ModelViewerPanel::upd_orbit_camera_controller()
{
    return private_data().upd_orbit_camera_controller();
}
void osc::ModelViewerPanel::set_model_state(
    const std::shared_ptr<opyn::ModelStatePair>& new_model_state)
{
    private_data().set_model_state(new_model_state);
}
void osc::ModelViewerPanel::impl_draw_content()
{
    private_data().draw_content();
}
void osc::ModelViewerPanel::impl_before_imgui_begin()
{
    ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f});
}
void osc::ModelViewerPanel::impl_after_imgui_begin()
{
    ui::pop_style_var();
}
