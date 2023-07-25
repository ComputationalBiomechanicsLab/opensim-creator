#include "ExportPointsPopup.hpp"

#include "OpenSimCreator/Bindings/SimTKHelpers.hpp"
#include "OpenSimCreator/Model/VirtualConstModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Formats/CSV.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/SetHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Widgets/StandardPopup.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/Wrap/PathWrapPoint.h>
#include <Simbody.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace
{
    constexpr osc::CStringView c_ExplanationText = "Exports the chosen points within the model, potentially with respect to a chosen frame, as a standard data file (CSV)";
    constexpr osc::CStringView c_OriginalFrameLabel = "(original frame)";

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

    bool IsVisibleInPointList(
        PointSelectorUiState const& uiState,
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        return
            osc::CanExtractPointInfoFrom(component, state) &&
            osc::ContainsSubstringCaseInsensitive(component.getName(), uiState.searchString);
    }

    void DrawExportPointsPopupDescriptionSection()
    {
        ImGui::Text("Description:");
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::TextWrapped("%s", c_ExplanationText.c_str());
        ImGui::PopStyleColor();
    }

    void DrawPointListElementHoverTooltip(
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        osc::BeginTooltip();
        ImGui::TextUnformatted(component.getName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", component.getConcreteClassName().c_str());

        if (std::optional<osc::PointInfo> const pointInfo = osc::TryExtractPointInfo(component, state))
        {
            ImGui::TextDisabled("Expressed In: %s", pointInfo->frameAbsPath.toString().c_str());
        }

        osc::EndTooltip();
    }

    void DrawPointListElement(
        PointSelectorUiState& uiState,
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        OSC_ASSERT(osc::CanExtractPointInfoFrom(component, state));

        std::string const absPath = osc::GetAbsolutePathString(component);

        bool selected = osc::Contains(uiState.selectedPointAbsPaths, absPath);
        if (ImGui::Checkbox(component.getName().c_str(), &selected))
        {
            if (selected)
            {
                uiState.selectedPointAbsPaths.insert(absPath);
            }
            else
            {
                uiState.selectedPointAbsPaths.erase(absPath);
            }
        }

        if (ImGui::IsItemHovered())
        {
            DrawPointListElementHoverTooltip(component, state);
        }
    }

    void DrawPointSelectionList(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        auto color = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
        color.w *= 0.5f;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
        bool const showingListBox = ImGui::BeginListBox("list");
        ImGui::PopStyleColor();

        if (showingListBox)
        {
            int imguiID = 0;
            for (OpenSim::Component const& component : model.getComponentList())
            {
                if (IsVisibleInPointList(uiState, component, state))
                {
                    ImGui::PushID(imguiID++);
                    DrawPointListElement(uiState, component, state);
                    ImGui::PopID();
                }
            }
            ImGui::EndListBox();
        }
    }

    void ActionSelectAllListedComponents(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        for (OpenSim::Component const& component : model.getComponentList())
        {
            if (IsVisibleInPointList(uiState, component, state))
            {
                uiState.selectedPointAbsPaths.insert(osc::GetAbsolutePathString(component));
            }
        }
    }

    void ActionDeselectAllListedComponents(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        for (OpenSim::Component const& component : model.getComponentList())
        {
            if (IsVisibleInPointList(uiState, component, state))
            {
                uiState.selectedPointAbsPaths.erase(osc::GetAbsolutePathString(component));
            }
        }
    }

    void ActionClearSelectedComponents(PointSelectorUiState& uiState)
    {
        uiState.selectedPointAbsPaths.clear();
    }

    void DrawSelectionManipulatorButtons(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        if (ImGui::Button("Select Listed"))
        {
            ActionSelectAllListedComponents(uiState, model, state);
        }

        ImGui::SameLine();

        if (ImGui::Button("De-Select Listed"))
        {
            ActionDeselectAllListedComponents(uiState, model, state);
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear Selection"))
        {
            ActionClearSelectedComponents(uiState);
        }
    }

    void DrawPointSelector(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        ImGui::Text("Which Points:");
        osc::InputString("search", uiState.searchString);
        DrawPointSelectionList(uiState, model, state);
        DrawSelectionManipulatorButtons(uiState, model, state);
    }

    OpenSim::Component const* TryGetMaybeSelectedFrameOrNullptr(
        FrameSelectorUiState const& uiState,
        OpenSim::Model const& model)
    {
        return uiState.maybeSelectedFrameAbsPath ?
            osc::FindComponent(model, *uiState.maybeSelectedFrameAbsPath) :
            nullptr;
    }

    std::string CalcComboLabel(
        FrameSelectorUiState const& uiState,
        OpenSim::Model const& model)
    {
        OpenSim::Component const* c = TryGetMaybeSelectedFrameOrNullptr(uiState, model);
        return c ? c->getName() : std::string{c_OriginalFrameLabel};
    }

    void DrawOriginalFrameSelectable(FrameSelectorUiState& uiState)
    {
        bool const selected = uiState.maybeSelectedFrameAbsPath != std::nullopt;
        if (ImGui::Selectable(c_OriginalFrameLabel.c_str(), selected))
        {
            uiState.maybeSelectedFrameAbsPath.reset();
        }
    }

    void DrawModelFrameSelectable(
        FrameSelectorUiState& uiState,
        OpenSim::Frame const& frame)
    {
        std::string const absPath = osc::GetAbsolutePathString(frame);
        bool const selected = uiState.maybeSelectedFrameAbsPath == absPath;

        if (ImGui::Selectable(frame.getName().c_str(), selected))
        {
            uiState.maybeSelectedFrameAbsPath = absPath;
        }
    }

    void DrawModelFrameSelectables(
        FrameSelectorUiState& uiState,
        OpenSim::Model const& model)
    {
        int imguiID = 0;
        for (OpenSim::Frame const& frame : model.getComponentList<OpenSim::Frame>())
        {
            ImGui::PushID(imguiID++);
            DrawModelFrameSelectable(uiState, frame);
            ImGui::PopID();
        }
    }

    void DrawFrameSelector(FrameSelectorUiState& uiState, OpenSim::Model const& model)
    {
        std::string const label = CalcComboLabel(uiState, model);

        ImGui::Text("Express Points In:");
        if (ImGui::BeginCombo("Frame", label.c_str()))
        {
            DrawOriginalFrameSelectable(uiState);
            DrawModelFrameSelectables(uiState, model);
            ImGui::EndCombo();
        }
    }

    void DrawOutputFormatEditor(OutputFormatEditorUiState& uiState)
    {
        ImGui::Checkbox("Export Point Names as Absolute Paths", &uiState.exportPointNamesAsAbsPaths);
        osc::DrawTooltipBodyOnlyIfItemHovered("If selected, the exported point name will be the full path to the point (e.g. `/forceset/somemuscle/geometrypath/pointname`), rather than just the name of the point (e.g. `pointname`)");
    }

    enum class ExportStepReturn {
        UserCancelled = 0,
        IoError,
        Done,
        NUM_OPTIONS,
    };

    std::optional<SimTK::Transform> TryGetTransformToReexpressPointsIn(
        OpenSim::Model const& model,
        SimTK::State const& state,
        std::optional<std::string> const& maybeAbsPathOfFrameToReexpressPointsIn)
    {
        if (!maybeAbsPathOfFrameToReexpressPointsIn)
        {
            return std::nullopt;  // caller doesn't want re-expression
        }

        OpenSim::Frame const* const frame = osc::FindComponent<OpenSim::Frame>(model, *maybeAbsPathOfFrameToReexpressPointsIn);
        if (!frame)
        {
            return std::nullopt;  // the selected frame doesn't exist in the model (bug?)
        }

        return frame->getTransformInGround(state).invert();
    }

    std::vector<std::string> GetSortedListOfOutputPointAbsPaths(
        std::unordered_set<std::string> const& unorderedPointAbsPaths,
        bool shouldExportPointsWithAbsPathNames)
    {
        std::vector<std::string> rv(unorderedPointAbsPaths.begin(), unorderedPointAbsPaths.end());
        if (shouldExportPointsWithAbsPathNames)
        {
            std::sort(rv.begin(), rv.end());
        }
        else
        {
            auto const isEndOfPathLexographicallyLess = [](std::string const& a, std::string const& b)
            {
                return osc::SubstringAfterLast(a, '/') < osc::SubstringAfterLast(b, '/');
            };
            std::sort(rv.begin(), rv.end(), isEndOfPathLexographicallyLess);
        }
        return rv;
    }

    glm::vec3 CalcReexpressedFrame(
        OpenSim::Model const& model,
        SimTK::State const& state,
        osc::PointInfo const& pointInfo,
        SimTK::Transform const& ground2otherFrame)
    {
        OpenSim::Frame const* const frame = osc::FindComponent<OpenSim::Frame>(model, pointInfo.frameAbsPath);
        if (!frame)
        {
            return pointInfo.location;  // cannot find frame (bug?)
        }

        return osc::ToVec3(ground2otherFrame * frame->getTransformInGround(state) * osc::ToSimTKVec3(pointInfo.location));
    }

    void TryWriteOneCSVDataRow(
        OpenSim::Model const& model,
        SimTK::State const& state,
        bool shouldExportPointsWithAbsPathNames,
        std::optional<SimTK::Transform> const& maybeGround2ReexpressedFrame,
        std::string const& pointAbsPath,
        std::ostream& out)
    {
        OpenSim::Component const* const c = osc::FindComponent(model, pointAbsPath);
        if (!c)
        {
            return;  // skip writing: point no longer exists in model
        }

        std::optional<osc::PointInfo> const pi = osc::TryExtractPointInfo(*c, state);
        if (!pi)
        {
            return;  // skip writing: cannot extract point info for the component
        }

        // else: compute position, name, etc. and emit as a CSV data row

        glm::vec3 const position = maybeGround2ReexpressedFrame ?
            CalcReexpressedFrame(model, state, *pi, *maybeGround2ReexpressedFrame) :
            pi->location;

        std::string const name = shouldExportPointsWithAbsPathNames ?
            osc::GetAbsolutePathString(*c) :
            c->getName();

        auto const columns = osc::to_array<std::string>(
        {
            name,
            std::to_string(position[0]),
            std::to_string(position[1]),
            std::to_string(position[2]),
        });

        osc::WriteCSVRow(out, columns);
    }

    void WritePointsAsCSVTo(
        OpenSim::Model const& model,
        SimTK::State const& state,
        std::unordered_set<std::string> const& pointAbsPaths,
        std::optional<std::string> const& maybeAbsPathOfFrameToReexpressPointsIn,
        bool shouldExportPointsWithAbsPathNames,
        std::ostream& out)
    {
        std::vector<std::string> const sortedRowAbsPaths = GetSortedListOfOutputPointAbsPaths(
            pointAbsPaths,
            shouldExportPointsWithAbsPathNames
        );

        std::optional<SimTK::Transform> const maybeGround2ReexpressedFrame = TryGetTransformToReexpressPointsIn(
            model,
            state,
            maybeAbsPathOfFrameToReexpressPointsIn
        );

        // write header row
        osc::WriteCSVRow(
            out,
            osc::to_array<std::string>({ "Name", "X", "Y", "Z" })
        );

        // write data rows
        for (std::string const& path : sortedRowAbsPaths)
        {
            TryWriteOneCSVDataRow(
                model,
                state,
                shouldExportPointsWithAbsPathNames,
                maybeGround2ReexpressedFrame,
                path,
                out
            );
        }
    }

    ExportStepReturn ActionPromptUserForSaveLocationAndExportPoints(
        OpenSim::Model const& model,
        SimTK::State const& state,
        std::unordered_set<std::string> const& pointAbsPaths,
        std::optional<std::string> const& maybeAbsPathOfFrameToReexpressPointsIn,
        bool shouldExportPointsWithAbsPathNames)
    {
        // prompt user to select a save location
        std::optional<std::filesystem::path> const saveLoc = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
        if (!saveLoc)
        {
            return ExportStepReturn::UserCancelled;
        }

        // open the save location for writing
        std::ofstream fOut;
        fOut.exceptions(std::ios_base::badbit | std::ios_base::failbit);
        fOut.open(*saveLoc);
        if (!fOut)
        {
            return ExportStepReturn::IoError;
        }

        WritePointsAsCSVTo(
            model,
            state,
            pointAbsPaths,
            maybeAbsPathOfFrameToReexpressPointsIn,
            shouldExportPointsWithAbsPathNames,
            fOut
        );

        return ExportStepReturn::Done;
    }
}

class osc::ExportPointsPopup::Impl final : public osc::StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        std::shared_ptr<VirtualConstModelStatePair const> model_) :

        StandardPopup{popupName_},
        m_Model{std::move(model_)}
    {
    }

private:
    void implDrawContent() final
    {
        OpenSim::Model const& model = m_Model->getModel();
        SimTK::State const& state = m_Model->getState();

        DrawExportPointsPopupDescriptionSection();
        ImGui::Separator();
        DrawPointSelector(m_PointSelectorState, model, state);
        ImGui::Separator();
        DrawFrameSelector(m_FrameSelectorState, model);
        ImGui::Separator();
        DrawOutputFormatEditor(m_OutputFormatState);
        ImGui::Separator();
        drawBottomButtons();
    }

    void drawBottomButtons()
    {
        if (ImGui::Button("Cancel"))
        {
            requestClose();
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_UPLOAD " Export to CSV"))
        {
            static_assert(osc::NumOptions<ExportStepReturn>() == 3, "review error handling");
            ExportStepReturn const rv = ActionPromptUserForSaveLocationAndExportPoints(
                m_Model->getModel(),
                m_Model->getState(),
                m_PointSelectorState.selectedPointAbsPaths,
                m_FrameSelectorState.maybeSelectedFrameAbsPath,
                m_OutputFormatState.exportPointNamesAsAbsPaths
            );
            if (rv == ExportStepReturn::Done)
            {
                requestClose();
            }
        }
    }

    std::shared_ptr<VirtualConstModelStatePair const> m_Model;
    PointSelectorUiState m_PointSelectorState;
    FrameSelectorUiState m_FrameSelectorState;
    OutputFormatEditorUiState m_OutputFormatState;
};


// public API

osc::ExportPointsPopup::ExportPointsPopup(
    std::string_view popupName,
    std::shared_ptr<VirtualConstModelStatePair const> model_) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(model_))}
{
}
osc::ExportPointsPopup::ExportPointsPopup(ExportPointsPopup&&) noexcept = default;
osc::ExportPointsPopup& osc::ExportPointsPopup::operator=(ExportPointsPopup&&) noexcept = default;
osc::ExportPointsPopup::~ExportPointsPopup() noexcept = default;

bool osc::ExportPointsPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ExportPointsPopup::implOpen()
{
    m_Impl->open();
}

void osc::ExportPointsPopup::implClose()
{
    m_Impl->close();
}

bool osc::ExportPointsPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::ExportPointsPopup::implDrawPopupContent()
{
    m_Impl->drawPopupContent();
}

void osc::ExportPointsPopup::implEndPopup()
{
    m_Impl->endPopup();
}
