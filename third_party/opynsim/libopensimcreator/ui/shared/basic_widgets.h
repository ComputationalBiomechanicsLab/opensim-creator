#pragma once

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/c_string_view.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace OpenSim
{
    class AbstractOutput;
    class Component;
    class Ellipsoid;
    class Frame;
    class Geometry;
    class Mesh;
    class Point;
    class Sphere;
    class Station;
}

namespace SimTK
{
    class State;
}

namespace opyn
{
    class CustomRenderingOptions;
    class ModelStatePair;
    class OpenSimDecorationOptions;
    class OutputExtractor;
    class OverlayDecorationOptions;
    struct ModelRendererParams;
}

namespace osc
{
    class IconCache;
    struct OrbitCameraController;
    class ParamBlock;
    class Rect;
    struct SceneDecoration;
    class SimulationModelStatePair;
    class UndoableModelStatePair;
    class Widget;

    // returns the recommended icon codepoint for the given component
    CStringView icon_for(const OpenSim::Component&);

    void draw_component_hover_tooltip(const OpenSim::Component&);
    void draw_nothing_right_clicked_context_menu_header();
    void draw_context_menu_header(CStringView title, CStringView subtitle);
    void draw_right_clicked_component_context_menu_header(const OpenSim::Component&);
    void draw_context_menu_separator();
    void draw_select_owner_menu(opyn::ModelStatePair&, const OpenSim::Component&);
    bool draw_request_output_menu_or_menu_item(
        const OpenSim::AbstractOutput& o,
        const std::function<void(opyn::SharedOutputExtractor)>& on_user_selection);
    bool draw_watch_output_menu(
        const OpenSim::Component&,
        const std::function<void(opyn::SharedOutputExtractor)>& on_user_selection);
    void draw_simulation_params(const ParamBlock&);
    void draw_search_bar(std::string&);
    void draw_output_name_column(
        const opyn::OutputExtractor& output,
        bool centered = true,
        SimulationModelStatePair* maybe_active_sate = nullptr);

    // draws a "With Respect to" menu that prompts the user to hover a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuOpened` when the user is hovering a frame's menu
    // (i.e. `ui::begin_menu($FRAME)` returned `true`)
    void draw_with_respect_to_menu_containing_menu_per_frame(
        const OpenSim::Component& root,
        const std::function<void(const OpenSim::Frame&)>& on_frame_menu_opened,
        const OpenSim::Frame* maybe_parent);

    // draws a "With Respect to" menu that prompts the user to click a frame
    // within the given component hierarchy (from `root`)
    //
    // calls `onFrameMenuItemClicked` when the user clicks the `ui::draw_menu_item`
    // associated with a frame
    void draw_with_respect_to_menu_containing_menu_item_per_frame(
        const OpenSim::Component& root,
        const std::function<void(const OpenSim::Frame&)>& on_frame_menu_item_clicked,
        const OpenSim::Frame* maybe_parent);

    void draw_point_translation_information_with_respect_to(
        const OpenSim::Frame&, const SimTK::State&, Vector3 location_in_ground);
    void draw_direction_information_with_repsect_to(
        const OpenSim::Frame&, const SimTK::State&, Vector3 direction_in_ground);
    void draw_frame_information_expressed_in(
        const OpenSim::Frame& parent, const SimTK::State&, const OpenSim::Frame&);

    // calculate menus
    enum class CalculateMenuFlags {
        None = 0,
        NoCalculatorIcon = 1 << 0,
    };
    constexpr bool operator&(CalculateMenuFlags lhs, CalculateMenuFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }
    bool begin_calculate_menu(CalculateMenuFlags = CalculateMenuFlags::None);
    void end_calculate_menu();
    void draw_calculate_position_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Point&,
        const OpenSim::Frame* maybe_parent);
    void draw_calculate_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Station&,
        CalculateMenuFlags = CalculateMenuFlags::None);
    void draw_calculate_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Point&,
        CalculateMenuFlags = CalculateMenuFlags::None);
    void draw_calculate_transform_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Frame&);
    void draw_calculate_origin_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Frame&);
    void draw_calculate_axis_directions_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Frame&);
    void draw_calculate_origin_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Sphere&);
    void draw_calculate_radius_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Sphere&);
    void draw_calculate_volume_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Sphere&);
    void draw_calculate_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Frame&,
        CalculateMenuFlags = CalculateMenuFlags::None);
    void draw_calculate_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Geometry&,
        CalculateMenuFlags = CalculateMenuFlags::None);
    void try_draw_calculate_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Component& selected,
        CalculateMenuFlags = CalculateMenuFlags::None);
    void draw_calculate_origin_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Ellipsoid&);
    void draw_calculate_radii_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Ellipsoid&);
    void draw_calculate_radii_directions_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Ellipsoid&);
    void draw_calculate_scaled_radii_directions_menu(
        const OpenSim::Component& root, const SimTK::State&, const OpenSim::Ellipsoid&);
    void draw_calculate_menu(
        const OpenSim::Component& root,
        const SimTK::State&,
        const OpenSim::Ellipsoid&,
        CalculateMenuFlags = CalculateMenuFlags::None);

    // basic wigetized parts of the 3D viewer
    bool draw_muscle_rendering_options_radio_buttions(opyn::OpenSimDecorationOptions&);
    bool draw_muscle_sizing_options_radio_buttons(opyn::OpenSimDecorationOptions&);
    bool draw_muscle_color_source_options_radio_buttons(opyn::OpenSimDecorationOptions&);
    bool draw_muscle_color_scaling_options_radio_buttons(opyn::OpenSimDecorationOptions&);
    bool draw_muscle_decoration_options_editor(opyn::OpenSimDecorationOptions&);
    bool draw_rendering_options_editor(opyn::CustomRenderingOptions&);
    bool draw_overlay_options_editor(opyn::OverlayDecorationOptions&);
    bool draw_custom_decoration_option_checkboxes(opyn::OpenSimDecorationOptions&);
    bool draw_orbit_camera_controller_editor(OrbitCameraController&);
    bool draw_advanced_params_editor(
        opyn::ModelRendererParams&, OrbitCameraController&, std::span<const SceneDecoration>);
    bool draw_visual_aids_context_menu_content(opyn::ModelRendererParams&);
    bool draw_viewer_top_button_row(
        opyn::ModelRendererParams&,
        std::span<const SceneDecoration>,
        IconCache&,
        const std::function<bool()>& draw_extra_elements = [] { return false; });
    bool draw_camera_control_buttons(
        opyn::ModelRendererParams&,
        OrbitCameraController&,
        std::span<const SceneDecoration>,
        const Rect&,
        const std::optional<AABB>& maybe_scene_aabb,
        IconCache&,
        Vector2 desired_top_centroid);
    bool draw_viewer_im_gui_overlays(
        opyn::ModelRendererParams&,
        OrbitCameraController&,
        std::span<const SceneDecoration>,
        std::optional<AABB>,
        const Rect&,
        IconCache&,
        const std::function<bool()>& draw_extra_elements_in_top = [] { return false; });

    // toolbar stuff
    // behaves the same as `ui::begin_panel` (i.e. you must call `ui::end_panel`)
    bool begin_toolbar(CStringView label, std::optional<Vector2> padding = {});
    void draw_new_model_button(Widget&);
    void draw_open_model_button_with_recent_files_dropdown(
        const std::function<void(std::optional<std::filesystem::path>)>&
            on_user_clicked_open_or_selected_file);
    void draw_open_model_button_with_recent_files_dropdown(Widget&);
    void draw_save_model_button(const std::shared_ptr<opyn::ModelStatePair>&);
    void draw_reload_model_button(UndoableModelStatePair&);
    void draw_undo_button(opyn::ModelStatePair&);
    void draw_redo_button(opyn::ModelStatePair&);
    void draw_undo_and_redo_buttons(opyn::ModelStatePair&);
    void draw_toggle_frames_button(opyn::ModelStatePair&, IconCache&);
    void draw_toggle_markers_button(opyn::ModelStatePair&, IconCache&);
    void draw_toggle_wrap_geometry_button(opyn::ModelStatePair&, IconCache&);
    void draw_toggle_contact_geometry_button(opyn::ModelStatePair&, IconCache&);
    void draw_toggle_forces_button(opyn::ModelStatePair&, IconCache&);
    void draw_all_decoration_toggle_buttons(opyn::ModelStatePair&, IconCache&);
    void draw_scene_scale_factor_editor_controls(opyn::ModelStatePair&);

    // mesh stuff
    void draw_mesh_export_context_menu_content(const opyn::ModelStatePair&, const OpenSim::Mesh&);
}
