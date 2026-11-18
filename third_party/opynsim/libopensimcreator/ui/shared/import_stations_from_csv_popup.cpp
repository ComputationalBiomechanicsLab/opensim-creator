#include "import_stations_from_csv_popup.h"

#include <libopensimcreator/documents/mesh_importer/mi_undoable_actions.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/documents/landmarks/landmark.h>
#include <libopynsim/documents/landmarks/landmark_helpers.h>
#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/formats/csv.h>
#include <liboscar/graphics/color.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/file_dialog_filter.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class osc::ImportStationsFromCSVPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ImportStationsFromCSVPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::function<void(ImportedData)> on_import,
        std::shared_ptr<const opyn::ModelStatePair> maybe_associated_model) :

        PopupPrivate{owner, parent, popup_name},
        on_import_callback_{std::move(on_import)},
        maybe_associated_model_{std::move(maybe_associated_model)}
    {
        set_modal(true);
    }

    void draw_content()
    {
        draw_help_text();
        ui::draw_vertical_spacer(0.25f);

        if (not maybe_import_path_) {
            draw_select_initial_file_state();
            ui::draw_vertical_spacer(0.75f);
        } else {
            ui::draw_separator();
            draw_landmark_entries();
            draw_warnings();

            ui::draw_vertical_spacer(0.25f);
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);
        }

        if (maybe_associated_model_) {
            ui::draw_separator();
            ui::draw_text("Associate landmarks with a frame in the model");
            if (ui::begin_combobox("Model frame", target_component_abs_path_)) {
                for (const auto& frame : maybe_associated_model_->get_model()
                                             .getComponentList<OpenSim::PhysicalFrame>()) {

                    const std::string abs_path = frame.getAbsolutePathString();
                    if (ui::draw_selectable(abs_path, abs_path == target_component_abs_path_)) {
                        target_component_abs_path_ = abs_path;
                    }
                }
                ui::end_combobox();
            }
        }

        draw_possibly_disabled_ok_or_cancel_buttons();
        ui::draw_vertical_spacer(0.5f);
    }

private:
    void draw_help_text()
    {
        ui::draw_text_wrapped(
            "Use this tool to import CSV data containing 3D locations as stations into the document. The CSV file should contain:");
        ui::draw_bullet_point();
        ui::draw_text_wrapped(
            "(optional) A header row of four columns, ideally labelled 'name', 'x', 'y', and 'z'");
        ui::draw_bullet_point();
        ui::draw_text_wrapped(
            "Data rows containing four columns: name (optional, string), x (number), y (number), and z (number)");
        ui::draw_vertical_spacer(0.5f);
        constexpr CStringView c_example_input_text =
            "name,x,y,z\nstationatground,0,0,0\nstation2,1.53,0.2,1.7\nstation3,3.0,2.0,0.0\n";
        ui::draw_text_wrapped("Example Input: ");
        ui::same_line();
        if (ui::draw_button(MSMICONS_COPY)) {
            set_clipboard_text(c_example_input_text);
        }
        ui::draw_tooltip_body_only_if_item_hovered("Copy example input to clipboard");
        ui::indent();
        ui::draw_text_wrapped(c_example_input_text);
        ui::unindent();
    }

    void draw_select_initial_file_state()
    {
        if (ui::draw_button_centered(MSMICONS_FILE " Select File")) {
            action_try_prompting_user_for_csv_file();
        }
    }

    void draw_landmark_entries()
    {
        if (not maybe_import_path_ or imported_landmarks_.empty()) {
            return;
        }

        ui::draw_text_centered(maybe_import_path_->string());
        ui::draw_text_centered(
            std::string{"("} + std::to_string(imported_landmarks_.size()) + " data rows)");

        ui::draw_vertical_spacer(0.2f);
        if (ui::begin_table(
                "##importtable",
                4,
                ui::TableFlag::ScrollY,
                {0.0f, 10.0f * ui::get_text_line_height_in_current_panel()})) {
            ui::table_setup_column("Name");
            ui::table_setup_column("X");
            ui::table_setup_column("Y");
            ui::table_setup_column("Z");
            ui::table_headers_row();

            int id = 0;
            for (const auto& station : imported_landmarks_) {
                ui::push_id(id++);
                ui::table_next_row();
                int column = 0;
                ui::table_set_column_index(column++);
                ui::draw_text(station.name);
                ui::table_set_column_index(column++);
                ui::draw_text("{}", station.position.x());
                ui::table_set_column_index(column++);
                ui::draw_text("{}", station.position.y());
                ui::table_set_column_index(column++);
                ui::draw_text("{}", station.position.z());
                ui::pop_id();
            }

            ui::end_table();
        }
        ui::draw_vertical_spacer(0.2f);

        if (ui::draw_button(MSMICONS_FILE " Select Different File")) {
            action_try_prompting_user_for_csv_file();
        }
        ui::same_line();
        if (ui::draw_button(MSMICONS_RECYCLE " Reload Same File")) {
            action_load_csv_file(*maybe_import_path_);
        }
    }

    void draw_warnings()
    {
        if (import_warnings_.empty()) {
            return;
        }

        ui::push_style_color(ui::ColorVar::Text, Color::orange());
        ui::draw_text(MSMICONS_EXCLAMATION " input file contains issues");
        ui::pop_style_color();

        if (ui::is_item_hovered()) {
            ui::begin_tooltip();
            ui::indent();
            int id = 0;
            for (const auto& warning : import_warnings_) {
                ui::push_id(id++);
                ui::draw_text(warning);
                ui::pop_id();
            }
            ui::end_tooltip();
        }
    }

    void draw_possibly_disabled_ok_or_cancel_buttons()
    {
        std::optional<CStringView> disabled_reason;
        if (not maybe_import_path_) {
            disabled_reason = "Cannot continue: nothing has been imported (select a file first)";
        }
        if (imported_landmarks_.empty()) {
            disabled_reason = "Cannot continue: there are no landmarks to import";
        }

        if (disabled_reason) {
            ui::begin_disabled();
        }
        if (ui::draw_button("OK")) {
            action_attach_result_to_model_graph();
            owner().close();
        }
        if (disabled_reason) {
            ui::end_disabled();
            if (ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
                ui::draw_tooltip_body_only(*disabled_reason);
            }
        }
        ui::same_line();
        if (ui::draw_button("Cancel")) {
            owner().close();
        }
    }

    void action_try_prompting_user_for_csv_file()
    {
        App::upd().prompt_user_to_select_file_async(
            [this, this_lifetime = lifetime_watcher()](const FileDialogResponse& response) {
                if (this_lifetime.expired()) {
                    return;  // This UI widget has expired, so nothing to load it into.
                }

                if (response.size() != 1) {
                    return;  // Error, cancellation, or user somehow selected >1 file.
                }

                action_load_csv_file(response.front());
            },
            {
                CSV::file_dialog_filter(),
                FileDialogFilter::all_files(),
            });
    }

    void action_load_csv_file(const std::filesystem::path& path)
    {
        maybe_import_path_ = path;
        imported_landmarks_.clear();
        import_warnings_.clear();

        std::ifstream ifs{path};
        if (not ifs) {
            std::stringstream ss;
            ss << path << ": could not load the given path";
            import_warnings_.push_back(std::move(ss).str());
            return;
        }

        std::vector<opyn::Landmark> lms;
        opyn::read_landmarks_from_csv(
            ifs,
            [&lms](opyn::Landmark&& lm) { lms.push_back(std::move(lm)); },
            [this](const opyn::CSVParseWarning& warning) {
                import_warnings_.push_back(to_string(warning));
            });
        imported_landmarks_ = generate_names(lms);
    }

    void action_attach_result_to_model_graph()
    {
        if (imported_landmarks_.empty()) {
            return;
        }

        on_import_callback_({
            .maybe_label =
                maybe_import_path_ ? maybe_import_path_->string() : std::optional<std::string>{},
            .landmarks = imported_landmarks_,
            .maybe_target_component_abs_path = target_component_abs_path_ == "/ground"
                                                   ? std::optional<std::string>{}
                                                   : target_component_abs_path_,
        });
    }

    std::function<void(ImportedData)> on_import_callback_;
    std::optional<std::filesystem::path> maybe_import_path_;
    std::vector<opyn::NamedLandmark> imported_landmarks_;
    std::vector<std::string> import_warnings_;
    std::shared_ptr<const opyn::ModelStatePair> maybe_associated_model_;
    std::string target_component_abs_path_ = "/ground";
};

osc::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(
    Widget* parent,
    std::string_view popup_name,
    std::function<void(ImportedData)> on_import,
    std::shared_ptr<const opyn::ModelStatePair> maybe_associated_model) :
    Popup{std::make_unique<Impl>(
        *this, parent, popup_name, std::move(on_import), std::move(maybe_associated_model))}
{}
void osc::ImportStationsFromCSVPopup::impl_draw_content()
{
    private_data().draw_content();
}
