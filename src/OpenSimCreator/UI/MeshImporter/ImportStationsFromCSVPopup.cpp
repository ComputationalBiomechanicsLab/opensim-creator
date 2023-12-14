#include "ImportStationsFromCSVPopup.hpp"

#include <OpenSimCreator/Documents/Landmarks/Landmark.hpp>
#include <OpenSimCreator/Documents/Landmarks/LandmarkHelpers.hpp>
#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Station.hpp>
#include <OpenSimCreator/Documents/MeshImporter/UndoableActions.hpp>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterSharedState.hpp>

#include <oscar/Formats/CSV.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

class osc::mi::ImportStationsFromCSVPopup::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        std::shared_ptr<MeshImporterSharedState> shared_) :

        StandardPopup{popupName_},
        m_Shared{std::move(shared_)}
    {
        setModal(true);
    }

private:
    void implDrawContent() final
    {
        drawHelpText();
        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});

        if (!m_MaybeImportPath)
        {
            drawSelectInitialFileState();
            ImGui::Dummy({0.0f, 0.75f*ImGui::GetTextLineHeight()});
        }
        else
        {
            ImGui::Separator();
            drawLandmarkEntries();
            drawWarnings();

            ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
            ImGui::Separator();
            ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});

        }
        drawPossiblyDisabledOkOrCancelButtons();
        ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});
    }

    void drawHelpText()
    {
        ImGui::TextWrapped("Use this tool to import CSV data containing 3D locations as stations into the mesh importer scene. The CSV file should contain:");
        ImGui::Bullet();
        ImGui::TextWrapped("A header row of four columns, ideally labelled 'name', 'x', 'y', and 'z'");
        ImGui::Bullet();
        ImGui::TextWrapped("Data rows containing four columns: name (string), x (number), y (number), and z (number)");
        ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});
        constexpr CStringView c_ExampleInputText = "name,x,y,z\nstationatground,0,0,0\nstation2,1.53,0.2,1.7\nstation3,3.0,2.0,0.0\n";
        ImGui::TextWrapped("Example Input: ");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_COPY))
        {
            SetClipboardText(c_ExampleInputText);
        }
        DrawTooltipBodyOnlyIfItemHovered("Copy example input to clipboard");
        ImGui::Indent();
        ImGui::TextWrapped("%s", c_ExampleInputText.c_str());
        ImGui::Unindent();
    }

    void drawSelectInitialFileState()
    {
        if (ButtonCentered(ICON_FA_FILE " Select File"))
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

        TextCentered(m_MaybeImportPath->string());
        TextCentered(std::string{"("} + std::to_string(m_ImportedLandmarks.size()) + " data rows)");

        ImGui::Dummy({0.0f, 0.2f*ImGui::GetTextLineHeight()});
        if (ImGui::BeginTable("##importtable", 4, ImGuiTableFlags_ScrollY, {0.0f, 10.0f*ImGui::GetTextLineHeight()}))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("X");
            ImGui::TableSetupColumn("Y");
            ImGui::TableSetupColumn("Z");
            ImGui::TableHeadersRow();

            int id = 0;
            for (auto const& station : m_ImportedLandmarks)
            {
                ImGui::PushID(id++);
                ImGui::TableNextRow();
                int column = 0;
                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(station.name.c_str());
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", station.position.x);
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", station.position.y);
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", station.position.z);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::Dummy({0.0f, 0.2f*ImGui::GetTextLineHeight()});

        if (ImGui::Button(ICON_FA_FILE " Select Different File"))
        {
            actionTryPromptingUserForCSVFile();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_RECYCLE " Reload Same File"))
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

        PushStyleColor(ImGuiCol_Text, Color::orange());
        ImGui::Text(ICON_FA_EXCLAMATION " input file contains issues");
        PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            BeginTooltip();
            ImGui::Indent();
            int id = 0;
            for (auto const& warning : m_ImportWarnings)
            {
                ImGui::PushID(id++);
                TextUnformatted(warning);
                ImGui::PopID();
            }
            EndTooltip();
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
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("OK"))
        {
            actionAttachResultToModelGraph();
            close();
        }
        if (disabledReason)
        {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                DrawTooltipBodyOnly(*disabledReason);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
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
            [this](lm::CSVParseWarning warning) { m_ImportWarnings.push_back(to_string(warning)); }
        );
        m_ImportedLandmarks = GenerateNames(lms);
    }

    void actionAttachResultToModelGraph()
    {
        if (m_ImportedLandmarks.empty())
        {
            return;
        }

        std::optional<std::string> label = m_MaybeImportPath ? m_MaybeImportPath->string() : std::optional<std::string>{};
        ActionImportLandmarksToModelGraph(
            m_Shared->updCommittableModelGraph(),
            m_ImportedLandmarks,
            label
        );
    }

    std::shared_ptr<MeshImporterSharedState> m_Shared;
    std::optional<std::filesystem::path> m_MaybeImportPath;
    std::vector<lm::NamedLandmark> m_ImportedLandmarks;
    std::vector<std::string> m_ImportWarnings;
};


osc::mi::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(
    std::string_view popupName_,
    std::shared_ptr<MeshImporterSharedState> const& state_) :
    m_Impl{std::make_unique<Impl>(popupName_, state_)}
{
}
osc::mi::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(ImportStationsFromCSVPopup&&) noexcept = default;
osc::mi::ImportStationsFromCSVPopup& osc::mi::ImportStationsFromCSVPopup::operator=(ImportStationsFromCSVPopup&&) noexcept = default;
osc::mi::ImportStationsFromCSVPopup::~ImportStationsFromCSVPopup() noexcept = default;

bool osc::mi::ImportStationsFromCSVPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::mi::ImportStationsFromCSVPopup::implOpen()
{
    m_Impl->open();
}

void osc::mi::ImportStationsFromCSVPopup::implClose()
{
    m_Impl->close();
}

bool osc::mi::ImportStationsFromCSVPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::mi::ImportStationsFromCSVPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::mi::ImportStationsFromCSVPopup::implEndPopup()
{
    m_Impl->endPopup();
}
