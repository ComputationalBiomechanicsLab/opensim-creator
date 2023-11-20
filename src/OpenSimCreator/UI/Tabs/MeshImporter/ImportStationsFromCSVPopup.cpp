#include "ImportStationsFromCSVPopup.hpp"

#include <OpenSimCreator/ModelGraph/StationEl.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterSharedState.hpp>

#include <oscar/Formats/CSV.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

class osc::ImportStationsFromCSVPopup::Impl final : public StandardPopup {
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
    struct StationDefinedInGround final {
        std::string name;
        Vec3 location;
    };

    struct StationsDefinedInGround final {
        std::vector<StationDefinedInGround> rows;
    };

    struct ImportedCSVData final {
        std::filesystem::path sourceDataPath;
        std::variant<StationsDefinedInGround> parsedData;
    };

    struct CSVImportError final {
        std::filesystem::path userSelectedPath;
        std::string message;
    };

    using CSVImportResult = std::variant<ImportedCSVData, CSVImportError>;

    struct RowParseError final {
        size_t lineNum;
        std::string errorMsg;
    };

    static std::variant<StationDefinedInGround, RowParseError> TryParseColumns(
        size_t lineNum,
        std::span<std::string const> columnsText)
    {
        if (columnsText.size() < 4)
        {
            return RowParseError{lineNum, "too few columns in this row (expecting at least 4)"};
        }

        std::string const& stationName = columnsText[0];

        std::optional<float> const maybeX = osc::FromCharsStripWhitespace(columnsText[1]);
        if (!maybeX)
        {
            return RowParseError{lineNum, "cannot parse X as a number"};
        }

        std::optional<float> const maybeY = osc::FromCharsStripWhitespace(columnsText[2]);
        if (!maybeY)
        {
            return RowParseError{lineNum, "cannot parse Y as a number"};
        }

        std::optional<float> const maybeZ = osc::FromCharsStripWhitespace(columnsText[3]);
        if (!maybeZ)
        {
            return RowParseError{lineNum, "cannot parse Z as a number"};
        }

        Vec3 const locationInGround = {*maybeX, *maybeY, *maybeZ};

        return StationDefinedInGround{stationName, locationInGround};
    }

    static std::string to_string(RowParseError const& e)
    {
        std::stringstream ss;
        ss << "line " << e.lineNum << ": " << e.errorMsg;
        return std::move(ss).str();
    }

    static bool IsWhitespaceRow(std::span<std::string const> cols)
    {
        return cols.size() == 1;
    }

    static CSVImportResult TryReadCSVInput(std::filesystem::path const& path, std::istream& input)
    {
        // input must contain at least one (header) row
        if (!osc::ReadCSVRow(input))
        {
            return CSVImportError{path, "cannot read a header row from the input (is the file empty?)"};
        }

        // then try to read each row as a data row, propagating errors
        // accordingly

        StationsDefinedInGround successfullyParsedStations;
        std::optional<RowParseError> maybeParseError;
        {
            size_t lineNum = 1;
            for (std::vector<std::string> row;
                !maybeParseError && osc::ReadCSVRowIntoVector(input, row);
                ++lineNum)
            {
                if (IsWhitespaceRow(row))
                {
                    continue;  // skip
                }

                // else: try parsing the row as a data row
                std::visit(Overload
                    {
                        [&successfullyParsedStations](StationDefinedInGround const& success)
                    {
                        successfullyParsedStations.rows.push_back(success);
                    },
                    [&maybeParseError](RowParseError const& fail)
                    {
                        maybeParseError = fail;
                    },
                    }, TryParseColumns(lineNum, row));
            }
        }

        if (maybeParseError)
        {
            return CSVImportError{path, to_string(*maybeParseError)};
        }
        else
        {
            return ImportedCSVData{path, std::move(successfullyParsedStations)};
        }
    }

    static CSVImportResult TryReadCSVFile(std::filesystem::path const& path)
    {
        std::ifstream f{path};
        if (!f)
        {
            return CSVImportError{path, "cannot open the provided file for reading"};
        }
        f.setf(std::ios_base::skipws);

        return TryReadCSVInput(path, f);
    }

    void implDrawContent() final
    {
        drawHelpText();

        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        if (m_MaybeImportResult.has_value())
        {
            ImGui::Separator();

            std::visit(Overload
                {
                    [this](ImportedCSVData const& data) { drawLoadedFileState(data); },
                    [this](CSVImportError const& error) { drawErrorLoadingFileState(error); }
                }, *m_MaybeImportResult);
        }
        else
        {
            drawSelectInitialFileState();
        }
        ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});
    }

    void drawHelpText()
    {
        ImGui::TextWrapped("Use this tool to import CSV data containing 3D locations as stations into the mesh importer scene. The CSV file should contain");
        ImGui::Bullet();
        ImGui::TextWrapped("A header row of four columns, ideally labelled 'name', 'x', 'y', and 'z'");
        ImGui::Bullet();
        ImGui::TextWrapped("Data rows containing four columns: name (string), x (number), y (number), and z (number)");

        constexpr CStringView c_ExampleInputText = "name,x,y,z\nstationatground,0,0,0\nstation2,1.53,0.2,1.7\nstation3,3.0,2.0,0.0\n";
        ImGui::TextWrapped("Example Input: ");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_COPY))
        {
            osc::SetClipboardText(c_ExampleInputText);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Copy example input to clipboard");
        ImGui::Indent();
        ImGui::TextWrapped("%s", c_ExampleInputText.c_str());
        ImGui::Unindent();
    }

    void drawSelectInitialFileState()
    {
        if (osc::ButtonCentered(ICON_FA_FILE " Select File"))
        {
            actionTryPromptingUserForCSVFile();
        }

        ImGui::Dummy({0.0f, 0.75f*ImGui::GetTextLineHeight()});

        drawDisabledOkCancelButtons("Cannot continue: nothing has been imported (select a file first)");
    }

    void drawErrorLoadingFileState(CSVImportError const& error)
    {
        ImGui::Text("Error loading %s: %s ", error.userSelectedPath.string().c_str(), error.message.c_str());
        if (ImGui::Button("Try Again (Select File)"))
        {
            actionTryPromptingUserForCSVFile();
        }

        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});

        drawDisabledOkCancelButtons("Cannot continue: there is an error in the imported data (try again)");
    }

    void drawDisabledOkCancelButtons(CStringView disabledReason)
    {
        ImGui::BeginDisabled();
        ImGui::Button("OK");
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            osc::DrawTooltipBodyOnly(disabledReason);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            close();
        }
    }

    void drawLoadedFileState(ImportedCSVData const& result)
    {
        std::visit(Overload
            {
                [this, &result](StationsDefinedInGround const& data) { drawLoadedFileStateData(result, data); },
            }, result.parsedData);

        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});

        if (ImGui::Button("OK"))
        {
            actionAttachResultToModelGraph(result);
            close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            close();
        }
    }

    void drawLoadedFileStateData(ImportedCSVData const& result, StationsDefinedInGround const& data)
    {
        osc::TextCentered(result.sourceDataPath.string());
        osc::TextCentered(std::string{"("} + std::to_string(data.rows.size()) + " data rows)");

        ImGui::Dummy({0.0f, 0.2f*ImGui::GetTextLineHeight()});
        if (ImGui::BeginTable("##importtable", 4, ImGuiTableFlags_ScrollY, {0.0f, 10.0f*ImGui::GetTextLineHeight()}))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("X");
            ImGui::TableSetupColumn("Y");
            ImGui::TableSetupColumn("Z");
            ImGui::TableHeadersRow();

            int id = 0;
            for (StationDefinedInGround const& row : data.rows)
            {
                ImGui::PushID(id++);
                ImGui::TableNextRow();
                int column = 0;
                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(row.name.c_str());
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", row.location.x);
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", row.location.y);
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%f", row.location.z);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::Dummy({0.0f, 0.2f*ImGui::GetTextLineHeight()});

        if (osc::ButtonCentered(ICON_FA_FILE " Select Different File"))
        {
            actionTryPromptingUserForCSVFile();
        }
    }

    void actionTryPromptingUserForCSVFile()
    {
        if (auto path = osc::PromptUserForFile("csv"))
        {
            m_MaybeImportResult = TryReadCSVFile(*path);
        }
    }

    void actionAttachResultToModelGraph(ImportedCSVData const& result)
    {
        std::visit(Overload
            {
                [this, &result](StationsDefinedInGround const& data) { actionAttachStationsInGroundToModelGraph(result, data); },
            }, result.parsedData);
    }

    void actionAttachStationsInGroundToModelGraph(
        ImportedCSVData const& result,
        StationsDefinedInGround const& data)
    {
        CommittableModelGraph& undoable = m_Shared->updCommittableModelGraph();

        ModelGraph& graph = undoable.updScratch();
        for (StationDefinedInGround const& station : data.rows)
        {
            graph.emplaceEl<StationEl>(
                UID{},
                ModelGraphIDs::Ground(),
                station.location,
                station.name
            );
        }

        std::stringstream ss;
        ss << "imported " << result.sourceDataPath;
        undoable.commitScratch(std::move(ss).str());
    }

    std::shared_ptr<MeshImporterSharedState> m_Shared;
    std::optional<CSVImportResult> m_MaybeImportResult;
};


osc::ImportStationsFromCSVPopup::ImportStationsFromCSVPopup(
    std::string_view popupName_,
    std::shared_ptr<MeshImporterSharedState> state_) :
    m_Impl{std::make_unique<Impl>(popupName_, state_)}
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
