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
        setModal(true);
    }

private:
    void implDrawContent() final
    {
        drawHelpText();
        ui::Dummy({0.0f, 0.25f*ui::GetTextLineHeight()});

        if (!m_MaybeImportPath)
        {
            drawSelectInitialFileState();
            ui::Dummy({0.0f, 0.75f*ui::GetTextLineHeight()});
        }
        else
        {
            ui::Separator();
            drawLandmarkEntries();
            drawWarnings();

            ui::Dummy({0.0f, 0.25f*ui::GetTextLineHeight()});
            ui::Separator();
            ui::Dummy({0.0f, 0.5f*ui::GetTextLineHeight()});

        }
        drawPossiblyDisabledOkOrCancelButtons();
        ui::Dummy({0.0f, 0.5f*ui::GetTextLineHeight()});
    }

    void drawHelpText()
    {
        ui::TextWrapped("Use this tool to import CSV data containing 3D locations as stations into the document. The CSV file should contain:");
        ui::Bullet();
        ui::TextWrapped("(optional) A header row of four columns, ideally labelled 'name', 'x', 'y', and 'z'");
        ui::Bullet();
        ui::TextWrapped("Data rows containing four columns: name (optional, string), x (number), y (number), and z (number)");
        ui::Dummy({0.0f, 0.5f*ui::GetTextLineHeight()});
        constexpr CStringView c_ExampleInputText = "name,x,y,z\nstationatground,0,0,0\nstation2,1.53,0.2,1.7\nstation3,3.0,2.0,0.0\n";
        ui::TextWrapped("Example Input: ");
        ui::SameLine();
        if (ui::Button(ICON_FA_COPY))
        {
            SetClipboardText(c_ExampleInputText);
        }
        ui::DrawTooltipBodyOnlyIfItemHovered("Copy example input to clipboard");
        ui::Indent();
        ui::TextWrapped(c_ExampleInputText);
        ui::Unindent();
    }

    void drawSelectInitialFileState()
    {
        if (ui::ButtonCentered(ICON_FA_FILE " Select File"))
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

        ui::TextCentered(m_MaybeImportPath->string());
        ui::TextCentered(std::string{"("} + std::to_string(m_ImportedLandmarks.size()) + " data rows)");

        ui::Dummy({0.0f, 0.2f*ui::GetTextLineHeight()});
        if (ui::BeginTable("##importtable", 4, ImGuiTableFlags_ScrollY, {0.0f, 10.0f*ui::GetTextLineHeight()}))
        {
            ui::TableSetupColumn("Name");
            ui::TableSetupColumn("X");
            ui::TableSetupColumn("Y");
            ui::TableSetupColumn("Z");
            ui::TableHeadersRow();

            int id = 0;
            for (auto const& station : m_ImportedLandmarks)
            {
                ui::PushID(id++);
                ui::TableNextRow();
                int column = 0;
                ui::TableSetColumnIndex(column++);
                ui::TextUnformatted(station.name);
                ui::TableSetColumnIndex(column++);
                ui::Text("%f", station.position.x);
                ui::TableSetColumnIndex(column++);
                ui::Text("%f", station.position.y);
                ui::TableSetColumnIndex(column++);
                ui::Text("%f", station.position.z);
                ui::PopID();
            }

            ui::EndTable();
        }
        ui::Dummy({0.0f, 0.2f*ui::GetTextLineHeight()});

        if (ui::Button(ICON_FA_FILE " Select Different File"))
        {
            actionTryPromptingUserForCSVFile();
        }
        ui::SameLine();
        if (ui::Button(ICON_FA_RECYCLE " Reload Same File"))
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

        ui::PushStyleColor(ImGuiCol_Text, Color::orange());
        ui::Text(ICON_FA_EXCLAMATION " input file contains issues");
        ui::PopStyleColor();

        if (ui::IsItemHovered())
        {
            ui::BeginTooltip();
            ui::Indent();
            int id = 0;
            for (auto const& warning : m_ImportWarnings)
            {
                ui::PushID(id++);
                ui::TextUnformatted(warning);
                ui::PopID();
            }
            ui::EndTooltip();
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
            ui::BeginDisabled();
        }
        if (ui::Button("OK"))
        {
            actionAttachResultToModelGraph();
            close();
        }
        if (disabledReason)
        {
            ui::EndDisabled();
            if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ui::DrawTooltipBodyOnly(*disabledReason);
            }
        }
        ui::SameLine();
        if (ui::Button("Cancel"))
        {
            close();
        }
    }

    void actionTryPromptingUserForCSVFile()
    {
        if (auto const path = PromptUserForFile("csv"))
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

bool osc::ImportStationsFromCSVPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ImportStationsFromCSVPopup::implOpen()
{
    m_Impl->open();
}

void osc::ImportStationsFromCSVPopup::implClose()
{
    m_Impl->close();
}

bool osc::ImportStationsFromCSVPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::ImportStationsFromCSVPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::ImportStationsFromCSVPopup::implEndPopup()
{
    m_Impl->endPopup();
}
