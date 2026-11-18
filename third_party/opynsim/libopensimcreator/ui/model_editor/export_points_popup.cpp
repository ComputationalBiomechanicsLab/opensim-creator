#include "export_points_popup.h"

#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/formats/csv.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr CStringView c_explanation_text =
        "Exports the chosen points within the model, potentially with respect to a chosen frame, as a standard data file (CSV)";
    constexpr CStringView c_original_frame_label = "(original frame)";

    struct PointSelectorUiState final {
        std::string searchString;
        std::unordered_set<std::string> selectedPointAbsPaths;
    };

    struct FrameSelectorUiState final {
        std::optional<std::string> maybeSelectedFrameAbsPath;
    };

    struct OutputFormatEditorUiState final {
        bool exportPointNamesAsAbsPaths = true;
    };

    struct UiState final {
        PointSelectorUiState pointSelector;
        FrameSelectorUiState frameSelector;
        OutputFormatEditorUiState outputFormat;
    };

    bool is_visible_in_point_list(
        const PointSelectorUiState& ui_state,
        const OpenSim::Component& component,
        const SimTK::State& state)
    {
        return opyn::can_extract_point_info_from(component, state) and
               contains_case_insensitive(component.getName(), ui_state.searchString);
    }

    void draw_export_points_popup_description_section()
    {
        ui::draw_text("Description");
        ui::draw_separator();
        ui::begin_disabled();
        ui::draw_text_wrapped(c_explanation_text);
        ui::end_disabled();
    }

    void draw_point_list_element_hover_tooltip(
        const OpenSim::Component& component, const SimTK::State& state)
    {
        ui::begin_tooltip();
        ui::draw_text(component.getName());
        ui::same_line();
        ui::draw_text_disabled(component.getConcreteClassName());

        if (const std::optional<opyn::PointInfo> point_info =
                opyn::try_extract_point_info(component, state)) {
            ui::draw_text_disabled("Expressed In: {}", point_info->frame_abs_path.toString());
        }

        ui::end_tooltip();
    }

    void draw_point_list_element(
        PointSelectorUiState& ui_state,
        const OpenSim::Component& component,
        const SimTK::State& state)
    {
        OSC_ASSERT(opyn::can_extract_point_info_from(component, state));

        const std::string abs_path = opyn::get_absolute_path_string(component);

        bool selected = ui_state.selectedPointAbsPaths.contains(abs_path);
        if (ui::draw_checkbox(component.getName(), &selected)) {
            if (selected) {
                ui_state.selectedPointAbsPaths.insert(abs_path);
            } else {
                ui_state.selectedPointAbsPaths.erase(abs_path);
            }
        }

        if (ui::is_item_hovered()) {
            draw_point_list_element_hover_tooltip(component, state);
        }
    }

    void draw_point_selection_list(
        PointSelectorUiState& ui_state, const OpenSim::Model& model, const SimTK::State& state)
    {
        auto color = ui::get_style_color(ui::ColorVar::FrameBg);
        color.a *= 0.5f;

        ui::push_style_color(ui::ColorVar::FrameBg, color);
        const bool showing_list_box = ui::begin_listbox("##PointsList");
        ui::pop_style_color();

        if (showing_list_box) {
            int imgui_id = 0;
            for (const OpenSim::Component& component : model.getComponentList()) {
                if (is_visible_in_point_list(ui_state, component, state)) {
                    ui::push_id(imgui_id++);
                    draw_point_list_element(ui_state, component, state);
                    ui::pop_id();
                }
            }
            ui::end_listbox();
        }
    }

    enum class SelectionState {
        Selected,
        NotSelected,
        NUM_OPTIONS,
    };

    void action_change_selection_state_if(
        PointSelectorUiState& ui_state,
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::function<bool(const OpenSim::Component&)>& predicate,
        SelectionState selection_state)
    {
        for (const OpenSim::Component& component : model.getComponentList()) {
            if (opyn::can_extract_point_info_from(component, state) && predicate(component)) {
                static_assert(num_options_v<SelectionState> == 2u);
                switch (selection_state) {
                case SelectionState::Selected:
                    ui_state.selectedPointAbsPaths.insert(
                        opyn::get_absolute_path_string(component));
                    break;
                case SelectionState::NotSelected:
                default:
                    ui_state.selectedPointAbsPaths.erase(opyn::get_absolute_path_string(component));
                    break;
                }
            }
        }
    }

    void draw_change_selection_state_of_points_expressed_in_menu_content(
        PointSelectorUiState& ui_state,
        const OpenSim::Model& model,
        const SimTK::State& state,
        SelectionState new_state_on_user_click)
    {
        for (const OpenSim::Frame& f : model.getComponentList<OpenSim::Frame>()) {
            if (ui::draw_menu_item(f.getName())) {
                const auto is_attached_to_frame = [path = opyn::get_absolute_path(f),
                                                   &state](const OpenSim::Component& c) {
                    if (const auto point_info = opyn::try_extract_point_info(c, state)) {
                        return point_info->frame_abs_path == path;
                    } else {
                        return false;
                    }
                };

                action_change_selection_state_if(
                    ui_state, model, state, is_attached_to_frame, new_state_on_user_click);
            }
        }
    }

    void draw_selection_state_modifier_menu_content(
        PointSelectorUiState& ui_state,
        const OpenSim::Model& model,
        const SimTK::State& state,
        SelectionState new_state_on_user_click)
    {
        if (ui::draw_menu_item("All")) {
            action_change_selection_state_if(
                ui_state,
                model,
                state,
                [](const OpenSim::Component&) { return true; },
                new_state_on_user_click);
        }

        if (ui::draw_menu_item("Listed (searched)")) {
            action_change_selection_state_if(
                ui_state,
                model,
                state,
                [&ui_state, &state](const OpenSim::Component& c) {
                    return is_visible_in_point_list(ui_state, c, state);
                },
                new_state_on_user_click);
        }

        if (ui::begin_menu("Expressed In")) {
            draw_change_selection_state_of_points_expressed_in_menu_content(
                ui_state, model, state, new_state_on_user_click);
            ui::end_menu();
        }
    }

    void draw_selection_manipulator_buttons(
        PointSelectorUiState& ui_state, const OpenSim::Model& model, const SimTK::State& state)
    {
        ui::draw_button("Select" MSMICONS_CARET_DOWN);
        if (ui::begin_popup_context_menu("##selectmenu", ui::PopupFlag::MouseButtonLeft)) {
            draw_selection_state_modifier_menu_content(
                ui_state, model, state, SelectionState::Selected);

            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button("De-Select" MSMICONS_CARET_DOWN);
        if (ui::begin_popup_context_menu("##deselectmenu", ui::PopupFlag::MouseButtonLeft)) {
            draw_selection_state_modifier_menu_content(
                ui_state, model, state, SelectionState::NotSelected);

            ui::end_popup();
        }
    }

    void draw_point_selector(
        PointSelectorUiState& ui_state, const OpenSim::Model& model, const SimTK::State& state)
    {
        ui::draw_text("Points");
        ui::draw_separator();
        ui::draw_string_input("search", ui_state.searchString);
        draw_point_selection_list(ui_state, model, state);
        draw_selection_manipulator_buttons(ui_state, model, state);
    }

    const OpenSim::Component* try_get_maybe_selected_frame_or_nullptr(
        const FrameSelectorUiState& ui_state, const OpenSim::Model& model)
    {
        return ui_state.maybeSelectedFrameAbsPath
                   ? opyn::find_component(model, *ui_state.maybeSelectedFrameAbsPath)
                   : nullptr;
    }

    std::string calc_combo_label(const FrameSelectorUiState& ui_state, const OpenSim::Model& model)
    {
        const OpenSim::Component* c = try_get_maybe_selected_frame_or_nullptr(ui_state, model);
        return c ? c->getName() : std::string{c_original_frame_label};
    }

    void draw_original_frame_selectable(FrameSelectorUiState& ui_state)
    {
        const bool selected = ui_state.maybeSelectedFrameAbsPath != std::nullopt;
        if (ui::draw_selectable(c_original_frame_label, selected)) {
            ui_state.maybeSelectedFrameAbsPath.reset();
        }
    }

    void draw_model_frame_selectable(FrameSelectorUiState& ui_state, const OpenSim::Frame& frame)
    {
        const std::string abs_path = opyn::get_absolute_path_string(frame);
        const bool selected = ui_state.maybeSelectedFrameAbsPath == abs_path;

        if (ui::draw_selectable(frame.getName(), selected)) {
            ui_state.maybeSelectedFrameAbsPath = abs_path;
        }
    }

    void draw_model_frame_selectables(FrameSelectorUiState& ui_state, const OpenSim::Model& model)
    {
        int imgui_id = 0;
        for (const OpenSim::Frame& frame : model.getComponentList<OpenSim::Frame>()) {
            ui::push_id(imgui_id++);
            draw_model_frame_selectable(ui_state, frame);
            ui::pop_id();
        }
    }

    void draw_frame_selector(FrameSelectorUiState& ui_state, const OpenSim::Model& model)
    {
        const std::string label = calc_combo_label(ui_state, model);
        if (ui::begin_combobox("Express Points In", label)) {
            draw_original_frame_selectable(ui_state);
            draw_model_frame_selectables(ui_state, model);
            ui::end_combobox();
        }
    }

    void draw_output_format_editor(OutputFormatEditorUiState& ui_state)
    {
        ui::draw_checkbox(
            "Export Point Names as Absolute Paths", &ui_state.exportPointNamesAsAbsPaths);
        ui::draw_tooltip_body_only_if_item_hovered(
            "If selected, the exported point name will be the full path to the point (e.g. `/forceset/somemuscle/geometrypath/pointname`), rather than just the name of the point (e.g. `pointname`)");
    }

    std::optional<SimTK::Transform> try_get_transform_to_reexpress_points_in(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::optional<std::string>& maybe_abs_path_of_frame_to_reexpress_points_in)
    {
        if (not maybe_abs_path_of_frame_to_reexpress_points_in) {
            return std::nullopt;  // caller doesn't want re-expression
        }

        const auto* const frame = opyn::find_component<OpenSim::Frame>(
            model, *maybe_abs_path_of_frame_to_reexpress_points_in);
        if (not frame) {
            return std::nullopt;  // the selected frame doesn't exist in the model (bug?)
        }

        return frame->getTransformInGround(state).invert();
    }

    std::vector<std::string> get_sorted_list_of_output_point_abs_paths(
        const std::unordered_set<std::string>& unordered_point_abs_paths,
        bool should_export_points_with_abs_path_names)
    {
        std::vector<std::string> rv(
            unordered_point_abs_paths.begin(), unordered_point_abs_paths.end());
        if (should_export_points_with_abs_path_names) {
            rgs::sort(rv);
        } else {
            rgs::sort(
                rv, rgs::less{}, [](const auto& path) { return substring_after_last(path, '/'); });
        }
        return rv;
    }

    Vector3 calc_reexpressed_frame(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const opyn::PointInfo& point_info,
        const SimTK::Transform& ground2other_frame)
    {
        const auto* const frame =
            opyn::find_component<OpenSim::Frame>(model, point_info.frame_abs_path);
        if (not frame) {
            return point_info.location;  // cannot find frame (bug?)
        }

        return to<Vector3>(
            ground2other_frame * frame->getTransformInGround(state) *
            to<SimTK::Vec3>(point_info.location));
    }

    void try_write_one_csv_data_row(
        const OpenSim::Model& model,
        const SimTK::State& state,
        bool should_export_points_with_abs_path_names,
        const std::optional<SimTK::Transform>& maybe_ground2_reexpressed_frame,
        const std::string& point_abs_path,
        std::ostream& out)
    {
        const OpenSim::Component* const c = opyn::find_component(model, point_abs_path);
        if (not c) {
            return;  // skip writing: point no longer exists in model
        }

        const std::optional<opyn::PointInfo> poi = opyn::try_extract_point_info(*c, state);
        if (not poi) {
            return;  // skip writing: cannot extract point info for the component
        }

        // else: compute position, name, etc. and emit as a CSV data row

        const Vector3 position =
            maybe_ground2_reexpressed_frame
                ? calc_reexpressed_frame(model, state, *poi, *maybe_ground2_reexpressed_frame)
                : poi->location;

        const std::string name = should_export_points_with_abs_path_names
                                     ? opyn::get_absolute_path_string(*c)
                                     : c->getName();

        const auto columns = std::to_array<std::string>({
            name,
            std::to_string(position.x()),
            std::to_string(position.y()),
            std::to_string(position.z()),
        });

        CSV::write_row(out, columns);
    }

    void write_points_as_csv_to(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::unordered_set<std::string>& point_abs_paths,
        const std::optional<std::string>& maybe_abs_path_of_frame_to_reexpress_points_in,
        bool should_export_points_with_abs_path_names,
        std::ostream& out)
    {
        const std::vector<std::string> sorted_row_abs_paths =
            get_sorted_list_of_output_point_abs_paths(
                point_abs_paths, should_export_points_with_abs_path_names);

        const std::optional<SimTK::Transform> maybe_ground2_reexpressed_frame =
            try_get_transform_to_reexpress_points_in(
                model, state, maybe_abs_path_of_frame_to_reexpress_points_in);

        // write header row
        CSV::write_row(out, std::to_array<std::string>({"Name", "X", "Y", "Z"}));

        // write data rows
        for (const std::string& path : sorted_row_abs_paths) {
            try_write_one_csv_data_row(
                model,
                state,
                should_export_points_with_abs_path_names,
                maybe_ground2_reexpressed_frame,
                path,
                out);
        }
    }

    void action_prompt_user_for_save_location_and_export_points(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::unordered_set<std::string>& point_abs_paths,
        const std::optional<std::string>& maybe_abs_path_of_frame_to_reexpress_points_in,
        bool should_export_points_with_abs_path_names)
    {
        // Pre-write the CSV in memory so that the asynchronous user prompt doesn't depend
        // on a bunch of UI state.
        std::stringstream ss;
        write_points_as_csv_to(
            model,
            state,
            point_abs_paths,
            maybe_abs_path_of_frame_to_reexpress_points_in,
            should_export_points_with_abs_path_names,
            ss);

        // Asynchronously prompt the user for a save location and write the CSV to it.
        App::upd().prompt_user_to_save_file_with_extension_async(
            [csv = std::move(ss).str()](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                std::ofstream ofs{*p};
                if (not ofs) {
                    log_error("{}: error opening file for writing", p->string());
                    return;
                }
                ofs << csv;
            },
            "csv");
    }
}

class osc::ExportPointsPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ExportPointsPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<const opyn::ModelStatePair> model) :

        PopupPrivate{owner, parent, popup_name},
        model_{std::move(model)}
    {}

    void draw_content()
    {
        const OpenSim::Model& model = model_->get_model();
        const SimTK::State& state = model_->get_state();

        draw_export_points_popup_description_section();
        ui::draw_vertical_spacer(0.5f);

        draw_point_selector(point_selector_state_, model, state);
        ui::draw_vertical_spacer(0.5f);

        ui::draw_text("Options");
        ui::draw_separator();
        draw_frame_selector(frame_selector_state_, model);
        draw_output_format_editor(output_format_state_);
        ui::draw_vertical_spacer(0.5f);

        draw_bottom_buttons();
    }

private:
    void draw_bottom_buttons()
    {
        if (ui::draw_button("Cancel")) {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button(MSMICONS_UPLOAD " Export to CSV")) {
            action_prompt_user_for_save_location_and_export_points(
                model_->get_model(),
                model_->get_state(),
                point_selector_state_.selectedPointAbsPaths,
                frame_selector_state_.maybeSelectedFrameAbsPath,
                output_format_state_.exportPointNamesAsAbsPaths);
        }
    }

    std::shared_ptr<const opyn::ModelStatePair> model_;
    PointSelectorUiState point_selector_state_;
    FrameSelectorUiState frame_selector_state_;
    OutputFormatEditorUiState output_format_state_;
};

osc::ExportPointsPopup::ExportPointsPopup(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<const opyn::ModelStatePair> model) :

    Popup{std::make_unique<Impl>(*this, parent, popup_name, std::move(model))}
{}
void osc::ExportPointsPopup::impl_draw_content()
{
    private_data().draw_content();
}
