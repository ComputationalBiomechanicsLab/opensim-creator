#include "ImportStationsFromCSVPopup.h"

#include <OpenSimCreator/Documents/Landmarks/Landmark.h>
#include <OpenSimCreator/Documents/Landmarks/LandmarkHelpers.h>
#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.h>
#include <OpenSimCreator/Documents/MeshImporter/UndoableActions.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/Widgets/StandardPopup.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class osc::ImportStationsFromCSVPopup::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        std::function<void(ImportedData)> onImport_) :

        StandardPopup{popupName_},
        m_OnImportCallback{std::move(onImport_)}
    {
        set_modal(true);
    }

private:
    void impl_draw_content() final
    {
        drawHelpText();
        ui::draw_dummy({0.0f, 0.25f*ui::get_text_line_height()});

        if (!m_MaybeImportPath)
        {
            drawSelectInitialFileState();
            ui::draw_dummy({0.0f, 0.75f*ui::get_text_line_height()});
        }
        else
        {
            ui::draw_separator();
            drawLandmarkEntries();
            drawWarnings();

            ui::draw_dummy({0.0f, 0.25f*ui::get_text_line_height()});
            ui::draw_separator();
            ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});

        }
        drawPossiblyDisabledOkOrCancelButtons();
        ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});
    }

    void drawHelpText()
    {
        ui::draw_text_wrapped("Use this tool to import CSV data containing 3D locations as stations into the document. The CSV file should contain:");
        ui::draw_bullet_point();
        ui::draw_text_wrapped("(optional) A header row of four columns, ideally labelled 'name', 'x', 'y', and 'z'");
        ui::draw_bullet_point();
        ui::draw_text_wrapped("Data rows containing four columns: name (optional, string), x (number), y (number), and z (number)");
        ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});
        constexpr CStringView c_ExampleInputText = "name,x,y,z\nstationatground,0,0,0\nstation2,1.53,0.2,1.7\nstation3,3.0,2.0,0.0\n";
        ui::draw_text_wrapped("Example Input: ");
        ui::same_line();
        if (ui::draw_button(ICON_FA_COPY))
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
        if (ui::draw_button_centered(ICON_FA_FILE " Select File"))
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

        ui::draw_dummy({0.0f, 0.2f*ui::get_text_line_height()});
        if (ui::begin_table("##importtable", 4, ImGuiTableFlags_ScrollY, {0.0f, 10.0f*ui::get_text_line_height()}))
        {
            ui::table_setup_column("Name");
            ui::table_setup_column("X");
            ui::table_setup_column("Y");
            ui::table_setup_column("Z");
            ui::table_headers_row();

            int id = 0;
            for (auto const& station : m_ImportedLandmarks)
            {
                ui::push_id(id++);
                ui::table_next_row();
                int column = 0;
                ui::table_set_column_index(column++);
                ui::draw_text_unformatted(station.name);
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
        ui::draw_dummy({0.0f, 0.2f*ui::get_text_line_height()});

        if (ui::draw_button(ICON_FA_FILE " Select Different File"))
        {
            actionTryPromptingUserForCSVFile();
        }
        ui::same_line();
        if (ui::draw_button(ICON_FA_RECYCLE " Reload Same File"))
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

        ui::push_style_color(ImGuiCol_Text, Color::orange());
        ui::draw_text(ICON_FA_EXCLAMATION " input file contains issues");
        ui::pop_style_color();

        if (ui::is_item_hovered())
        {
            ui::begin_tooltip();
            ui::indent();
            int id = 0;
            for (auto const& warning : m_ImportWarnings)
            {
                ui::push_id(id++);
                ui::draw_text_unformatted(warning);
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

        if (disabledReason)
        {
            ui::begin_disabled();
        }
        if (ui::draw_button("OK"))
        {
            actionAttachResultToModelGraph();
            close();
        }
        if (disabledReason)
        {
            ui::end_disabled();
            if (ui::is_item_hovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ui::draw_tooltip_body_only(*disabledReason);
            }
        }
        ui::same_line();
        if (ui::draw_button("Cancel"))
        {
            close();
        }
    }

    void actionTryPromptingUserForCSVFile()
    {
        if (auto const path = prompt_user_to_select_file({"csv"}))
        {
            actionLoadCSVFile(*path);
        }
    }

    void actionLoadCSVFile(std::filesystem::path const& path)
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
            [&lms](lm::Landmark&& lm) { lms.push_back(lm); },
            [this](lm::CSVParseWarning const& warning) { m_ImportWarnings.push_back(to_string(warning)); }
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
        });
    }

    std::function<void(ImportedData)> m_OnImportCallback;
    std::optional<std::filesystem::path> m_MaybeImportPath;
    std::vector<lm::NamedLandmark> m_ImportedLandmarks;
    std::vector<std::string> m_ImportWarnings;
};


osc::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(
    std::string_view popupName_,
    std::function<void(ImportedData)> onImport) :
    m_Impl{std::make_unique<Impl>(popupName_, std::move(onImport))}
{
}
osc::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(ImportStationsFromCSVPopup&&) noexcept = default;
osc::ImportStationsFromCSVPopup& osc::ImportStationsFromCSVPopup::operator=(ImportStationsFromCSVPopup&&) noexcept = default;
osc::ImportStationsFromCSVPopup::~ImportStationsFromCSVPopup() noexcept = default;

bool osc::ImportStationsFromCSVPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::ImportStationsFromCSVPopup::impl_open()
{
    m_Impl->open();
}

void osc::ImportStationsFromCSVPopup::impl_close()
{
    m_Impl->close();
}

bool osc::ImportStationsFromCSVPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::ImportStationsFromCSVPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::ImportStationsFromCSVPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
