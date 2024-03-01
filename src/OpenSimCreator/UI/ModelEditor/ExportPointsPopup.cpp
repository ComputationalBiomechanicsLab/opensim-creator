#include "ExportPointsPopup.h"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Formats/CSV.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>
#include <Simbody.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

using namespace osc;

namespace
{
    constexpr CStringView c_ExplanationText = "Exports the chosen points within the model, potentially with respect to a chosen frame, as a standard data file (CSV)";
    constexpr CStringView c_OriginalFrameLabel = "(original frame)";

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
            CanExtractPointInfoFrom(component, state) &&
            ContainsCaseInsensitive(component.getName(), uiState.searchString);
    }

    void DrawExportPointsPopupDescriptionSection()
    {
        ui::Text("Description");
        ui::Separator();
        ui::BeginDisabled();
        ui::TextWrapped(c_ExplanationText);
        ui::EndDisabled();
        ui::PopStyleColor();
    }

    void DrawPointListElementHoverTooltip(
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        ui::BeginTooltip();
        ui::TextUnformatted(component.getName());
        ui::SameLine();
        ui::TextDisabled(component.getConcreteClassName());

        if (std::optional<PointInfo> const pointInfo = TryExtractPointInfo(component, state))
        {
            ui::TextDisabled("Expressed In: %s", pointInfo->frameAbsPath.toString().c_str());
        }

        ui::EndTooltip();
    }

    void DrawPointListElement(
        PointSelectorUiState& uiState,
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        OSC_ASSERT(CanExtractPointInfoFrom(component, state));

        std::string const absPath = GetAbsolutePathString(component);

        bool selected = uiState.selectedPointAbsPaths.contains(absPath);
        if (ui::Checkbox(component.getName(), &selected))
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

        if (ui::IsItemHovered())
        {
            DrawPointListElementHoverTooltip(component, state);
        }
    }

    void DrawPointSelectionList(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        auto color = ui::GetStyle().Colors[ImGuiCol_FrameBg];
        color.w *= 0.5f;

        ui::PushStyleColor(ImGuiCol_FrameBg, color);
        bool const showingListBox = ui::BeginListBox("##PointsList");
        ui::PopStyleColor();

        if (showingListBox)
        {
            int imguiID = 0;
            for (OpenSim::Component const& component : model.getComponentList())
            {
                if (IsVisibleInPointList(uiState, component, state))
                {
                    ui::PushID(imguiID++);
                    DrawPointListElement(uiState, component, state);
                    ui::PopID();
                }
            }
            ui::EndListBox();
        }
    }

    enum class SelectionState {
        Selected,
        NotSelected,
        NUM_OPTIONS,
    };

    void ActionChangeSelectionStateIf(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state,
        std::function<bool(OpenSim::Component const&)> const& predicate,
        SelectionState selectionState)
    {
        for (OpenSim::Component const& component : model.getComponentList())
        {
            if (CanExtractPointInfoFrom(component, state) && predicate(component))
            {
                static_assert(NumOptions<SelectionState>() == 2u);
                switch (selectionState)
                {
                case SelectionState::Selected:
                    uiState.selectedPointAbsPaths.insert(GetAbsolutePathString(component));
                    break;
                case SelectionState::NotSelected:
                default:
                    uiState.selectedPointAbsPaths.erase(GetAbsolutePathString(component));
                    break;
                }
            }
        }
    }

    void DrawChangeSelectionStateOfPointsExpressedInMenuContent(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state,
        SelectionState newStateOnUserClick)
    {
        for (OpenSim::Frame const& f : model.getComponentList<OpenSim::Frame>())
        {
            if (ui::MenuItem(f.getName()))
            {
                auto const isAttachedToFrame = [path = GetAbsolutePath(f), &state](OpenSim::Component const& c)
                {
                    if (auto const pointInfo = TryExtractPointInfo(c, state))
                    {
                        return pointInfo->frameAbsPath == path;
                    }
                    else
                    {
                        return false;
                    }
                };

                ActionChangeSelectionStateIf(
                    uiState,
                    model,
                    state,
                    isAttachedToFrame,
                    newStateOnUserClick
                );
            }
        }
    }

    void DrawSelectionStateModifierMenuContent(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state,
        SelectionState newStateOnUserClick)
    {
        if (ui::MenuItem("All"))
        {
            ActionChangeSelectionStateIf(
                uiState,
                model,
                state,
                [](OpenSim::Component const&) { return true; },
                newStateOnUserClick
            );
        }

        if (ui::MenuItem("Listed (searched)"))
        {
            ActionChangeSelectionStateIf(
                uiState,
                model,
                state,
                [&uiState, &state](OpenSim::Component const& c)
                {
                    return IsVisibleInPointList(uiState, c, state);
                },
                newStateOnUserClick
            );
        }

        if (ui::BeginMenu("Expressed In"))
        {
            DrawChangeSelectionStateOfPointsExpressedInMenuContent(
                uiState,
                model,
                state,
                newStateOnUserClick
            );
            ui::EndMenu();
        }
    }

    void DrawSelectionManipulatorButtons(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        ui::Button("Select" ICON_FA_CARET_DOWN);
        if (ui::BeginPopupContextItem("##selectmenu", ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawSelectionStateModifierMenuContent(
                uiState,
                model,
                state,
                SelectionState::Selected
            );

            ui::EndPopup();
        }

        ui::SameLine();

        ui::Button("De-Select" ICON_FA_CARET_DOWN);
        if (ui::BeginPopupContextItem("##deselectmenu", ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawSelectionStateModifierMenuContent(
                uiState,
                model,
                state,
                SelectionState::NotSelected
            );

            ui::EndPopup();
        }
    }

    void DrawPointSelector(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        ui::Text("Points");
        ui::Separator();
        ui::InputString("search", uiState.searchString);
        DrawPointSelectionList(uiState, model, state);
        DrawSelectionManipulatorButtons(uiState, model, state);
    }

    OpenSim::Component const* TryGetMaybeSelectedFrameOrNullptr(
        FrameSelectorUiState const& uiState,
        OpenSim::Model const& model)
    {
        return uiState.maybeSelectedFrameAbsPath ?
            FindComponent(model, *uiState.maybeSelectedFrameAbsPath) :
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
        if (ui::Selectable(c_OriginalFrameLabel, selected))
        {
            uiState.maybeSelectedFrameAbsPath.reset();
        }
    }

    void DrawModelFrameSelectable(
        FrameSelectorUiState& uiState,
        OpenSim::Frame const& frame)
    {
        std::string const absPath = GetAbsolutePathString(frame);
        bool const selected = uiState.maybeSelectedFrameAbsPath == absPath;

        if (ui::Selectable(frame.getName(), selected))
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
            ui::PushID(imguiID++);
            DrawModelFrameSelectable(uiState, frame);
            ui::PopID();
        }
    }

    void DrawFrameSelector(FrameSelectorUiState& uiState, OpenSim::Model const& model)
    {
        std::string const label = CalcComboLabel(uiState, model);
        if (ui::BeginCombo("Express Points In", label))
        {
            DrawOriginalFrameSelectable(uiState);
            DrawModelFrameSelectables(uiState, model);
            ui::EndCombo();
        }
    }

    void DrawOutputFormatEditor(OutputFormatEditorUiState& uiState)
    {
        ui::Checkbox("Export Point Names as Absolute Paths", &uiState.exportPointNamesAsAbsPaths);
        ui::DrawTooltipBodyOnlyIfItemHovered("If selected, the exported point name will be the full path to the point (e.g. `/forceset/somemuscle/geometrypath/pointname`), rather than just the name of the point (e.g. `pointname`)");
    }

    enum class ExportStepReturn {
        UserCancelled,
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

        auto const* const frame = FindComponent<OpenSim::Frame>(model, *maybeAbsPathOfFrameToReexpressPointsIn);
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
            auto const isComponentNameLexographicallyLess = [](std::string const& a, std::string const& b)
            {
                return SubstringAfterLast(a, '/') < SubstringAfterLast(b, '/');
            };
            std::sort(rv.begin(), rv.end(), isComponentNameLexographicallyLess);
        }
        return rv;
    }

    Vec3 CalcReexpressedFrame(
        OpenSim::Model const& model,
        SimTK::State const& state,
        PointInfo const& pointInfo,
        SimTK::Transform const& ground2otherFrame)
    {
        auto const* const frame = FindComponent<OpenSim::Frame>(model, pointInfo.frameAbsPath);
        if (!frame)
        {
            return pointInfo.location;  // cannot find frame (bug?)
        }

        return ToVec3(ground2otherFrame * frame->getTransformInGround(state) * ToSimTKVec3(pointInfo.location));
    }

    void TryWriteOneCSVDataRow(
        OpenSim::Model const& model,
        SimTK::State const& state,
        bool shouldExportPointsWithAbsPathNames,
        std::optional<SimTK::Transform> const& maybeGround2ReexpressedFrame,
        std::string const& pointAbsPath,
        std::ostream& out)
    {
        OpenSim::Component const* const c = FindComponent(model, pointAbsPath);
        if (!c)
        {
            return;  // skip writing: point no longer exists in model
        }

        std::optional<PointInfo> const poi = TryExtractPointInfo(*c, state);
        if (!poi)
        {
            return;  // skip writing: cannot extract point info for the component
        }

        // else: compute position, name, etc. and emit as a CSV data row

        Vec3 const position = maybeGround2ReexpressedFrame ?
            CalcReexpressedFrame(model, state, *poi, *maybeGround2ReexpressedFrame) :
            poi->location;

        std::string const name = shouldExportPointsWithAbsPathNames ?
            GetAbsolutePathString(*c) :
            c->getName();

        auto const columns = std::to_array<std::string>(
        {
            name,
            std::to_string(position[0]),
            std::to_string(position[1]),
            std::to_string(position[2]),
        });

        WriteCSVRow(out, columns);
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
        WriteCSVRow(
            out,
            std::to_array<std::string>({ "Name", "X", "Y", "Z" })
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
        std::optional<std::filesystem::path> const saveLoc = PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
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

class osc::ExportPointsPopup::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        std::shared_ptr<IConstModelStatePair const> model_) :

        StandardPopup{popupName_},
        m_Model{std::move(model_)}
    {
    }

private:
    void implDrawContent() final
    {
        OpenSim::Model const& model = m_Model->getModel();
        SimTK::State const& state = m_Model->getState();

        float const sectionSpacing = 0.5f*ui::GetTextLineHeight();

        DrawExportPointsPopupDescriptionSection();
        ui::Dummy({0.0f, sectionSpacing});

        DrawPointSelector(m_PointSelectorState, model, state);
        ui::Dummy({0.0f, sectionSpacing});

        ui::Text("Options");
        ui::Separator();
        DrawFrameSelector(m_FrameSelectorState, model);
        DrawOutputFormatEditor(m_OutputFormatState);
        ui::Dummy({0.0f, sectionSpacing});

        drawBottomButtons();
    }

    void drawBottomButtons()
    {
        if (ui::Button("Cancel"))
        {
            requestClose();
        }

        ui::SameLine();

        if (ui::Button(ICON_FA_UPLOAD " Export to CSV"))
        {
            static_assert(NumOptions<ExportStepReturn>() == 3, "review error handling");
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

    std::shared_ptr<IConstModelStatePair const> m_Model;
    PointSelectorUiState m_PointSelectorState;
    FrameSelectorUiState m_FrameSelectorState;
    OutputFormatEditorUiState m_OutputFormatState;
};


// public API

osc::ExportPointsPopup::ExportPointsPopup(
    std::string_view popupName,
    std::shared_ptr<IConstModelStatePair const> model_) :

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

void osc::ExportPointsPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::ExportPointsPopup::implEndPopup()
{
    m_Impl->endPopup();
}
