#include "choose_components_editor_layer.h"

#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/choose_components_editor_layer_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_state.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/graphics/open_sim_graphics_helpers.h>
#include <libopynsim/graphics/overlay_decoration_generator.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/maths/bvh.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/string_name.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;

namespace
{
    // top-level shared state for the "choose components" layer
    struct ChooseComponentsEditorLayerSharedState final {

        explicit ChooseComponentsEditorLayerSharedState(
            std::shared_ptr<opyn::ModelStatePair> model,
            ChooseComponentsEditorLayerParameters parameters) :

            model{std::move(model)},
            popupParams{std::move(parameters)}
        {}

        std::shared_ptr<SceneCache> meshCache = App::singleton<SceneCache>(App::resource_loader());
        std::shared_ptr<opyn::ModelStatePair> model;
        ChooseComponentsEditorLayerParameters popupParams;
        opyn::ModelRendererParams renderParams;
        StringName hoveredComponent;
        std::unordered_set<StringName> alreadyChosenComponents;
        bool shouldClosePopup = false;
    };

    // grouping of scene (3D) decorations and an associated scene BVH
    struct BVHedDecorations final {
        void clear()
        {
            decorations.clear();
            bvh.clear();
        }

        std::vector<SceneDecoration> decorations;
        BVH bvh;
    };

    // generates scene decorations for the "choose components" layer
    void generate_choose_components_decorations(
        const ChooseComponentsEditorLayerSharedState& state, BVHedDecorations& out)
    {
        out.clear();

        const auto on_model_decoration =
            [&state, &out](const OpenSim::Component& component, SceneDecoration&& decoration) {
                // update flags based on path
                const StringName abs_path = opyn::get_absolute_path_string_name(component);
                if (state.popupParams.components_being_assigned_to.contains(abs_path) or
                    state.alreadyChosenComponents.contains(abs_path)) {

                    decoration.flags |= SceneDecorationFlag::RimHighlight0;
                }
                if (abs_path == state.hoveredComponent) {
                    decoration.flags |= SceneDecorationFlag::RimHighlight1;
                }

                if (state.popupParams.can_choose_item(component)) {
                    decoration.id = abs_path;
                } else if (std::holds_alternative<Color>(decoration.shading)) {
                    std::get<Color>(decoration.shading).a *= 0.2f;  // fade non-selectable objects
                }

                out.decorations.push_back(std::move(decoration));
            };

        generate_model_decorations(
            *state.meshCache,
            state.model->get_model(),
            state.model->get_state(),
            state.renderParams.decoration_options,
            state.model->get_fixup_scale_factor(),
            on_model_decoration);

        update_scene_bvh(out.decorations, out.bvh);

        const auto on_overlay_decoration = [&](SceneDecoration&& decoration) {
            out.decorations.push_back(std::move(decoration));
        };

        generate_overlay_decorations(
            *state.meshCache,
            state.renderParams.overlay_options,
            out.bvh,
            state.model->get_fixup_scale_factor(),
            on_overlay_decoration);
    }
}

class osc::ChooseComponentsEditorLayer::Impl final {
public:
    Impl(
        std::shared_ptr<opyn::ModelStatePair> model,
        ChooseComponentsEditorLayerParameters parameters) :

        state_{std::move(model), std::move(parameters)},
        renderer_{*App::singleton<SceneCache>(App::resource_loader())}
    {}

    bool handle_keyboard_inputs(
        ModelViewerPanelParameters& params, ModelViewerPanelState& state) const
    {
        const bool controller_updated = ui::update_orbit_controller_from_keyboard_inputs(
            params.upd_orbit_camera_controller(),
            params.get_camera(),
            state.viewport_ui_rect,
            decorations_.bvh.bounds());
        if (controller_updated) {
            params.get_orbit_camera_controller().update_camera(params.upd_camera());
        }
        return controller_updated;
    }

    bool handle_mouse_inputs(ModelViewerPanelParameters& params, ModelViewerPanelState& state)
    {
        bool rv = ui::update_orbit_controller_from_mouse_inputs(
            params.upd_orbit_camera_controller(),
            params.get_camera(),
            state.viewport_ui_rect.dimensions());
        if (rv) {
            params.get_orbit_camera_controller().update_camera(params.upd_camera());
        }

        if (ui::is_mouse_dragging_with_any_button_down()) {
            state_.hoveredComponent = {};
        }

        if (is_left_click_released_without_dragging_) {
            rv = try_toggle_hover() || rv;
        }

        return rv;
    }

    void on_draw(ModelViewerPanelParameters& panel_params, ModelViewerPanelState& panel_state)
    {
        const bool layer_is_hovered = ui::is_panel_hovered(ui::HoveredFlag::RootAndChildPanels);

        // update this layer's state from provided state
        state_.renderParams = panel_params.get_render_params();
        is_left_click_released_without_dragging_ =
            ui::is_mouse_released_without_dragging(MouseButton::Left);
        is_right_click_released_without_dragging_ =
            ui::is_mouse_released_without_dragging(MouseButton::Right);
        if (ui::is_key_released(Key::Escape)) {
            state_.shouldClosePopup = true;
        }

        // generate decorations + rendering params
        generate_choose_components_decorations(state_, decorations_);
        const SceneRendererParams renderer_parameters = opyn::calc_scene_renderer_params(
            state_.renderParams,
            panel_state.viewport_ui_rect.dimensions(),
            App::settings().get_value<float>("graphics/render_scale", 1.0f) *
                App::get().main_window_device_pixel_ratio(),
            App::get().anti_aliasing_level(),
            state_.model->get_fixup_scale_factor());

        // render to a texture (no caching)
        renderer_.render(decorations_.decorations, renderer_parameters);

        // blit texture as ImGui image
        ui::draw_image(renderer_.upd_render_texture(), panel_state.viewport_ui_rect.dimensions());

        // do hovertest
        if (layer_is_hovered) {
            const std::optional<SceneCollision> collision = opyn::get_closest_collision(
                decorations_.bvh,
                *state_.meshCache,
                decorations_.decorations,
                state_.renderParams.camera,
                ui::get_mouse_ui_position(),
                panel_state.viewport_ui_rect);
            if (collision) {
                state_.hoveredComponent = collision->decoration_id;
            } else {
                state_.hoveredComponent = {};
            }
        }

        // show tooltip
        if (const OpenSim::Component* c =
                opyn::find_component(state_.model->get_model(), state_.hoveredComponent)) {
            draw_component_hover_tooltip(*c);
        }

        // show header
        ui::set_cursor_ui_position(panel_state.viewport_ui_rect.ypd_top_left() + Vector2{10.0f});
        ui::draw_text("{} (ESC to cancel)", state_.popupParams.popup_header_text);

        // handle completion state (i.e. user selected enough components)
        if (state_.alreadyChosenComponents.size() ==
            state_.popupParams.num_components_user_must_choose) {

            state_.popupParams.on_user_finished_choosing(state_.alreadyChosenComponents);
            state_.shouldClosePopup = true;
        }

        // draw cancellation button
        {
            ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});

            constexpr CStringView cancellation_button_text = MSMICONS_ARROW_LEFT " Cancel (ESC)";
            const Vector2 margin = {25.0f, 25.0f};
            const Vector2 button_dims = ui::calc_button_size(cancellation_button_text);
            const Vector2 button_top_left =
                panel_state.viewport_ui_rect.ypd_bottom_right() - (button_dims + margin);
            ui::set_cursor_ui_position(button_top_left);
            if (ui::draw_button(cancellation_button_text)) {
                state_.shouldClosePopup = true;
            }

            ui::pop_style_var();
        }
    }

    float get_background_alpha() const { return 1.0f; }

    bool should_close() const { return state_.shouldClosePopup; }

    bool try_toggle_hover()
    {
        const auto& abs_path = state_.hoveredComponent;
        const OpenSim::Component* component =
            opyn::find_component(state_.model->get_model(), abs_path);

        if (not component) {
            return false;  // nothing hovered
        }
        if (state_.popupParams.components_being_assigned_to.contains(abs_path)) {
            return false;  // cannot be selected
        }
        if (auto it = state_.alreadyChosenComponents.find(abs_path);
            it != state_.alreadyChosenComponents.end()) {

            state_.alreadyChosenComponents.erase(it);
            return true;  // de-selected
        }
        if (state_.alreadyChosenComponents.size() <
                state_.popupParams.num_components_user_must_choose and
            state_.popupParams.can_choose_item(*component)) {

            state_.alreadyChosenComponents.insert(abs_path);
            return true;  // selected
        }

        return false;  // don't know how to handle
    }

    ChooseComponentsEditorLayerSharedState state_;
    BVHedDecorations decorations_;
    SceneRenderer renderer_;
    bool is_left_click_released_without_dragging_ = false;
    bool is_right_click_released_without_dragging_ = false;
};

osc::ChooseComponentsEditorLayer::ChooseComponentsEditorLayer(
    std::shared_ptr<opyn::ModelStatePair> model, ChooseComponentsEditorLayerParameters parameters) :

    impl_{std::make_unique<Impl>(std::move(model), std::move(parameters))}
{}
osc::ChooseComponentsEditorLayer::ChooseComponentsEditorLayer(
    ChooseComponentsEditorLayer&&) noexcept = default;
osc::ChooseComponentsEditorLayer& osc::ChooseComponentsEditorLayer::operator=(
    ChooseComponentsEditorLayer&&) noexcept = default;
osc::ChooseComponentsEditorLayer::~ChooseComponentsEditorLayer() noexcept = default;

bool osc::ChooseComponentsEditorLayer::impl_handle_keyboard_inputs(
    ModelViewerPanelParameters& params, ModelViewerPanelState& state)
{
    return impl_->handle_keyboard_inputs(params, state);
}

bool osc::ChooseComponentsEditorLayer::impl_handle_mouse_inputs(
    ModelViewerPanelParameters& params, ModelViewerPanelState& state)
{
    return impl_->handle_mouse_inputs(params, state);
}

void osc::ChooseComponentsEditorLayer::impl_on_draw(
    ModelViewerPanelParameters& params, ModelViewerPanelState& state)
{
    impl_->on_draw(params, state);
}

float osc::ChooseComponentsEditorLayer::impl_get_background_alpha() const
{
    return impl_->get_background_alpha();
}

bool osc::ChooseComponentsEditorLayer::impl_should_close() const
{
    return impl_->should_close();
}
