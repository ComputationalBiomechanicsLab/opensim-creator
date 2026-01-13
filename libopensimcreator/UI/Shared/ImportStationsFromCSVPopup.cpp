#include "ImportStationsFromCSVPopup.h"

#include <libopensimcreator/Documents/Landmarks/Landmark.h>
#include <libopensimcreator/Documents/Landmarks/LandmarkHelpers.h>
#include <libopensimcreator/Documents/Landmarks/NamedLandmark.h>
#include <libopensimcreator/Documents/MeshImporter/UndoableActions.h>
#include <libopensimcreator/Platform/msmicons.h>

#include <liboscar/formats/CSV.h>
#include <liboscar/graphics/Color.h>
#include <liboscar/platform/App.h>
#include <liboscar/platform/FileDialogFilter.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/popups/Popup.h>
#include <liboscar/ui/popups/PopupPrivate.h>
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
        std::string_view popupName_,
        std::function<void(ImportedData)> onImport_,
        std::shared_ptr<const IModelStatePair> maybeAssociatedModel_) :

        PopupPrivate{owner, parent, popupName_},
        m_OnImportCallback{std::move(onImport_)},
        m_MaybeAssociatedModel{std::move(maybeAssociatedModel_)}
    {
        set_modal(true);
    }

    void draw_content()
    {
        drawHelpText();
        ui::draw_vertical_spacer(0.25f);

        if (not m_MaybeImportPath) {
            drawSelectInitialFileState();
            ui::draw_vertical_spacer(0.75f);
        }
        else {
            ui::draw_separator();
            drawLandmarkEntries();
            drawWarnings();

            ui::draw_vertical_spacer(0.25f);
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);

        }

        if (m_MaybeAssociatedModel) {
            ui::draw_separator();
            ui::draw_text("Associate landmarks with a frame in the model");
            if (ui::begin_combobox("Model frame", m_TargetComponentAbsPath)) {
                for (const auto& frame : m_MaybeAssociatedModel->getModel().getComponentList<OpenSim::PhysicalFrame>()) {
                    const std::string absPath = frame.getAbsolutePathString();
                    if (ui::draw_selectable(absPath, absPath == m_TargetComponentAbsPath)) {
                        m_TargetComponentAbsPath = absPath;
                    }
                }
                ui::end_combobox();
            }
        }

        drawPossiblyDisabledOkOrCancelButtons();
        ui::draw_vertical_spacer(0.5f);
    }

private:
    void drawHelpText()
    {
        ui::draw_text_wrapped("Use this tool to import CSV data containing 3D locations as stations into the document. The CSV file should contain:");
        ui::draw_bullet_point();
        ui::draw_text_wrapped("(optional) A header row of four columns, ideally labelled 'name', 'x', 'y', and 'z'");
        ui::draw_bullet_point();
        ui::draw_text_wrapped("Data rows containing four columns: name (optional, string), x (number), y (number), and z (number)");
        ui::draw_vertical_spacer(0.5f);
        constexpr CStringView c_ExampleInputText = "name,x,y,z\nstationatground,0,0,0\nstation2,1.53,0.2,1.7\nstation3,3.0,2.0,0.0\n";
        ui::draw_text_wrapped("Example Input: ");
        ui::same_line();
        if (ui::draw_button(MSMICONS_COPY))
        {
            set_clipboard_text(c_ExampleInputText);
        }
        ui::draw_tooltip_body_only_if_item_hovered("Copy example input to clipboard");
        ui::indent();
        ui::draw_text_wrapped(c_ExampleInputText);
        ui::unindent();
    }

    void drawSelectInitialFileState()
    {
        if (ui::draw_button_centered(MSMICONS_FILE " Select File"))
        {
            actionTryPromptingUserForCSVFile();
        }
    }

    void drawLandmarkEntries()
    {
        if (!m_MaybeImportPath || m_ImportedLandmarks.empty())
        {
            return;
        }

        ui::draw_text_centered(m_MaybeImportPath->string());
        ui::draw_text_centered(std::string{"("} + std::to_string(m_ImportedLandmarks.size()) + " data rows)");

        ui::draw_vertical_spacer(0.2f);
        if (ui::begin_table("##importtable", 4, ui::TableFlag::ScrollY, {0.0f, 10.0f*ui::get_text_line_height_in_current_panel()}))
        {
            ui::table_setup_column("Name");
            ui::table_setup_column("X");
            ui::table_setup_column("Y");
            ui::table_setup_column("Z");
            ui::table_headers_row();

            int id = 0;
            for (const auto& station : m_ImportedLandmarks)
            {
                ui::push_id(id++);
                ui::table_next_row();
                int column = 0;
                ui::table_set_column_index(column++);
                ui::draw_text(station.name);
                ui::table_set_column_index(column++);
                ui::draw_text("%f", station.position.x);
                ui::table_set_column_index(column++);
                ui::draw_text("%f", station.position.y);
                ui::table_set_column_index(column++);
                ui::draw_text("%f", station.position.z);
                ui::pop_id();
            }

            ui::end_table();
        }
        ui::draw_vertical_spacer(0.2f);

        if (ui::draw_button(MSMICONS_FILE " Select Different File"))
        {
            actionTryPromptingUserForCSVFile();
        }
        ui::same_line();
        if (ui::draw_button(MSMICONS_RECYCLE " Reload Same File"))
        {
            actionLoadCSVFile(*m_MaybeImportPath);
        }
    }

    void drawWarnings()
    {
        if (m_ImportWarnings.empty())
        {
            return;
        }

        ui::push_style_color(ui::ColorVar::Text, Color::orange());
        ui::draw_text(MSMICONS_EXCLAMATION " input file contains issues");
        ui::pop_style_color();

        if (ui::is_item_hovered())
        {
            ui::begin_tooltip();
            ui::indent();
            int id = 0;
            for (const auto& warning : m_ImportWarnings)
            {
                ui::push_id(id++);
                ui::draw_text(warning);
                ui::pop_id();
            }
            ui::end_tooltip();
        }
    }

    void drawPossiblyDisabledOkOrCancelButtons()
    {
        std::optional<CStringView> disabledReason;
        if (!m_MaybeImportPath)
        {
            disabledReason = "Cannot continue: nothing has been imported (select a file first)";
        }
        if (m_ImportedLandmarks.empty())
        {
            disabledReason = "Cannot continue: there are no landmarks to import";
        }

        if (disabledReason) {
            ui::begin_disabled();
        }
        if (ui::draw_button("OK")) {
            actionAttachResultToModelGraph();
            owner().close();
        }
        if (disabledReason) {
            ui::end_disabled();
            if (ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
                ui::draw_tooltip_body_only(*disabledReason);
            }
        }
        ui::same_line();
        if (ui::draw_button("Cancel")) {
            owner().close();
        }
    }

    void actionTryPromptingUserForCSVFile()
    {
        App::upd().prompt_user_to_select_file_async(
            [this, this_lifetime = lifetime_watcher()](const FileDialogResponse& response)
            {
                if (this_lifetime.expired()) {
                    return;  // This UI widget has expired, so nothing to load it into.
                }

                if (response.size() != 1) {
                    return;  // Error, cancellation, or user somehow selected >1 file.
                }

                actionLoadCSVFile(response.front());
            },
            {
                CSV::file_dialog_filter(),
                FileDialogFilter::all_files(),
            }
        );
    }

    void actionLoadCSVFile(const std::filesystem::path& path)
    {
        m_MaybeImportPath = path;
        m_ImportedLandmarks.clear();
        m_ImportWarnings.clear();

        std::ifstream ifs{path};
        if (!ifs)
        {
            std::stringstream ss;
            ss << path << ": could not load the given path";
            m_ImportWarnings.push_back(std::move(ss).str());
            return;
        }

        std::vector<lm::Landmark> lms;
        lm::ReadLandmarksFromCSV(
            ifs,
            [&lms](lm::Landmark&& lm) { lms.push_back(std::move(lm)); },
            [this](const lm::CSVParseWarning& warning) { m_ImportWarnings.push_back(to_string(warning)); }
        );
        m_ImportedLandmarks = GenerateNames(lms);
    }

    void actionAttachResultToModelGraph()
    {
        if (m_ImportedLandmarks.empty())
        {
            return;
        }

        m_OnImportCallback({
            .maybeLabel = m_MaybeImportPath ? m_MaybeImportPath->string() : std::optional<std::string>{},
            .landmarks = m_ImportedLandmarks,
            .maybeTargetComponentAbsPath = m_TargetComponentAbsPath == "/ground" ? std::optional<std::string>{} : m_TargetComponentAbsPath,
        });
    }

    std::function<void(ImportedData)> m_OnImportCallback;
    std::optional<std::filesystem::path> m_MaybeImportPath;
    std::vector<lm::NamedLandmark> m_ImportedLandmarks;
    std::vector<std::string> m_ImportWarnings;
    std::shared_ptr<const IModelStatePair> m_MaybeAssociatedModel;
    std::string m_TargetComponentAbsPath = "/ground";
};

osc::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(
    Widget* parent,
    std::string_view popupName_,
    std::function<void(ImportedData)> onImport,
    std::shared_ptr<const IModelStatePair> maybeAssociatedModel) :
    Popup{std::make_unique<Impl>(*this, parent, popupName_, std::move(onImport), std::move(maybeAssociatedModel))}
{}
void osc::ImportStationsFromCSVPopup::impl_draw_content() { private_data().draw_content(); }
