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
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

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
            contains_case_insensitive(component.getName(), uiState.searchString);
    }

    void DrawExportPointsPopupDescriptionSection()
    {
        ui::draw_text("Description");
        ui::draw_separator();
        ui::begin_disabled();
        ui::draw_text_wrapped(c_ExplanationText);
        ui::end_disabled();
        ui::pop_style_color();
    }

    void DrawPointListElementHoverTooltip(
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        ui::begin_tooltip();
        ui::draw_text_unformatted(component.getName());
        ui::same_line();
        ui::draw_text_disabled(component.getConcreteClassName());

        if (std::optional<PointInfo> const pointInfo = TryExtractPointInfo(component, state))
        {
            ui::draw_text_disabled("Expressed In: %s", pointInfo->frameAbsPath.toString().c_str());
        }

        ui::end_tooltip();
    }

    void DrawPointListElement(
        PointSelectorUiState& uiState,
        OpenSim::Component const& component,
        SimTK::State const& state)
    {
        OSC_ASSERT(CanExtractPointInfoFrom(component, state));

        std::string const absPath = GetAbsolutePathString(component);

        bool selected = uiState.selectedPointAbsPaths.contains(absPath);
        if (ui::draw_checkbox(component.getName(), &selected))
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

        if (ui::is_item_hovered())
        {
            DrawPointListElementHoverTooltip(component, state);
        }
    }

    void DrawPointSelectionList(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        auto color = ui::get_style_color(ImGuiCol_FrameBg);
        color.a *= 0.5f;

        ui::push_style_color(ImGuiCol_FrameBg, color);
        bool const showingListBox = ui::begin_listbox("##PointsList");
        ui::pop_style_color();

        if (showingListBox)
        {
            int imguiID = 0;
            for (OpenSim::Component const& component : model.getComponentList())
            {
                if (IsVisibleInPointList(uiState, component, state))
                {
                    ui::push_id(imguiID++);
                    DrawPointListElement(uiState, component, state);
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
                static_assert(num_options<SelectionState>() == 2u);
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
            if (ui::draw_menu_item(f.getName()))
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
        if (ui::draw_menu_item("All"))
        {
            ActionChangeSelectionStateIf(
                uiState,
                model,
                state,
                [](OpenSim::Component const&) { return true; },
                newStateOnUserClick
            );
        }

        if (ui::draw_menu_item("Listed (searched)"))
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

        if (ui::begin_menu("Expressed In"))
        {
            DrawChangeSelectionStateOfPointsExpressedInMenuContent(
                uiState,
                model,
                state,
                newStateOnUserClick
            );
            ui::end_menu();
        }
    }

    void DrawSelectionManipulatorButtons(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        ui::draw_button("Select" ICON_FA_CARET_DOWN);
        if (ui::begin_popup_context_menu("##selectmenu", ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawSelectionStateModifierMenuContent(
                uiState,
                model,
                state,
                SelectionState::Selected
            );

            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button("De-Select" ICON_FA_CARET_DOWN);
        if (ui::begin_popup_context_menu("##deselectmenu", ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawSelectionStateModifierMenuContent(
                uiState,
                model,
                state,
                SelectionState::NotSelected
            );

            ui::end_popup();
        }
    }

    void DrawPointSelector(
        PointSelectorUiState& uiState,
        OpenSim::Model const& model,
        SimTK::State const& state)
    {
        ui::draw_text("Points");
        ui::draw_separator();
        ui::draw_string_input("search", uiState.searchString);
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
        if (ui::draw_selectable(c_OriginalFrameLabel, selected))
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

        if (ui::draw_selectable(frame.getName(), selected))
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
            ui::push_id(imguiID++);
            DrawModelFrameSelectable(uiState, frame);
            ui::pop_id();
        }
    }

    void DrawFrameSelector(FrameSelectorUiState& uiState, OpenSim::Model const& model)
    {
        std::string const label = CalcComboLabel(uiState, model);
        if (ui::begin_combobox("Express Points In", label))
        {
            DrawOriginalFrameSelectable(uiState);
            DrawModelFrameSelectables(uiState, model);
            ui::end_combobox();
        }
    }

    void DrawOutputFormatEditor(OutputFormatEditorUiState& uiState)
    {
        ui::draw_checkbox("Export Point Names as Absolute Paths", &uiState.exportPointNamesAsAbsPaths);
        ui::draw_tooltip_body_only_if_item_hovered("If selected, the exported point name will be the full path to the point (e.g. `/forceset/somemuscle/geometrypath/pointname`), rather than just the name of the point (e.g. `pointname`)");
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
        if (shouldExportPointsWithAbsPathNames) {
            rgs::sort(rv);
        }
        else {
            rgs::sort(rv, rgs::less{}, [](auto const& path) { return substring_after_last(path, '/'); });
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

        write_csv_row(out, columns);
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
        write_csv_row(
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
    void impl_draw_content() final
    {
        OpenSim::Model const& model = m_Model->getModel();
        SimTK::State const& state = m_Model->getState();

        float const sectionSpacing = 0.5f*ui::get_text_line_height();

        DrawExportPointsPopupDescriptionSection();
        ui::draw_dummy({0.0f, sectionSpacing});

        DrawPointSelector(m_PointSelectorState, model, state);
        ui::draw_dummy({0.0f, sectionSpacing});

        ui::draw_text("Options");
        ui::draw_separator();
        DrawFrameSelector(m_FrameSelectorState, model);
        DrawOutputFormatEditor(m_OutputFormatState);
        ui::draw_dummy({0.0f, sectionSpacing});

        drawBottomButtons();
    }

    void drawBottomButtons()
    {
        if (ui::draw_button("Cancel"))
        {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button(ICON_FA_UPLOAD " Export to CSV"))
        {
            static_assert(num_options<ExportStepReturn>() == 3, "review error handling");
            ExportStepReturn const rv = ActionPromptUserForSaveLocationAndExportPoints(
                m_Model->getModel(),
                m_Model->getState(),
                m_PointSelectorState.selectedPointAbsPaths,
                m_FrameSelectorState.maybeSelectedFrameAbsPath,
                m_OutputFormatState.exportPointNamesAsAbsPaths
            );
            if (rv == ExportStepReturn::Done)
            {
                request_close();
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

bool osc::ExportPointsPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::ExportPointsPopup::impl_open()
{
    m_Impl->open();
}

void osc::ExportPointsPopup::impl_close()
{
    m_Impl->close();
}

bool osc::ExportPointsPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::ExportPointsPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::ExportPointsPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
