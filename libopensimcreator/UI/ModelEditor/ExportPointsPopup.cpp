#include "ExportPointsPopup.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/Utils/SimTKConverters.h>

#include <liboscar/Formats/CSV.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Popups/Popup.h>
#include <liboscar/UI/Popups/PopupPrivate.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
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
        const PointSelectorUiState& uiState,
        const OpenSim::Component& component,
        const SimTK::State& state)
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
        const OpenSim::Component& component,
        const SimTK::State& state)
    {
        ui::begin_tooltip();
        ui::draw_text(component.getName());
        ui::same_line();
        ui::draw_text_disabled(component.getConcreteClassName());

        if (const std::optional<PointInfo> pointInfo = TryExtractPointInfo(component, state)) {
            ui::draw_text_disabled("Expressed In: %s", pointInfo->frameAbsPath.toString().c_str());
        }

        ui::end_tooltip();
    }

    void DrawPointListElement(
        PointSelectorUiState& uiState,
        const OpenSim::Component& component,
        const SimTK::State& state)
    {
        OSC_ASSERT(CanExtractPointInfoFrom(component, state));

        const std::string absPath = GetAbsolutePathString(component);

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
        const OpenSim::Model& model,
        const SimTK::State& state)
    {
        auto color = ui::get_style_color(ui::ColorVar::FrameBg);
        color.a *= 0.5f;

        ui::push_style_color(ui::ColorVar::FrameBg, color);
        const bool showingListBox = ui::begin_listbox("##PointsList");
        ui::pop_style_color();

        if (showingListBox)
        {
            int imguiID = 0;
            for (const OpenSim::Component& component : model.getComponentList())
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
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::function<bool(const OpenSim::Component&)>& predicate,
        SelectionState selectionState)
    {
        for (const OpenSim::Component& component : model.getComponentList())
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
        const OpenSim::Model& model,
        const SimTK::State& state,
        SelectionState newStateOnUserClick)
    {
        for (const OpenSim::Frame& f : model.getComponentList<OpenSim::Frame>())
        {
            if (ui::draw_menu_item(f.getName()))
            {
                const auto isAttachedToFrame = [path = GetAbsolutePath(f), &state](const OpenSim::Component& c)
                {
                    if (const auto pointInfo = TryExtractPointInfo(c, state))
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
        const OpenSim::Model& model,
        const SimTK::State& state,
        SelectionState newStateOnUserClick)
    {
        if (ui::draw_menu_item("All"))
        {
            ActionChangeSelectionStateIf(
                uiState,
                model,
                state,
                [](const OpenSim::Component&) { return true; },
                newStateOnUserClick
            );
        }

        if (ui::draw_menu_item("Listed (searched)"))
        {
            ActionChangeSelectionStateIf(
                uiState,
                model,
                state,
                [&uiState, &state](const OpenSim::Component& c)
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
        const OpenSim::Model& model,
        const SimTK::State& state)
    {
        ui::draw_button("Select" OSC_ICON_CARET_DOWN);
        if (ui::begin_popup_context_menu("##selectmenu", ui::PopupFlag::MouseButtonLeft))
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

        ui::draw_button("De-Select" OSC_ICON_CARET_DOWN);
        if (ui::begin_popup_context_menu("##deselectmenu", ui::PopupFlag::MouseButtonLeft))
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
        const OpenSim::Model& model,
        const SimTK::State& state)
    {
        ui::draw_text("Points");
        ui::draw_separator();
        ui::draw_string_input("search", uiState.searchString);
        DrawPointSelectionList(uiState, model, state);
        DrawSelectionManipulatorButtons(uiState, model, state);
    }

    const OpenSim::Component* TryGetMaybeSelectedFrameOrNullptr(
        const FrameSelectorUiState& uiState,
        const OpenSim::Model& model)
    {
        return uiState.maybeSelectedFrameAbsPath ?
            FindComponent(model, *uiState.maybeSelectedFrameAbsPath) :
            nullptr;
    }

    std::string CalcComboLabel(
        const FrameSelectorUiState& uiState,
        const OpenSim::Model& model)
    {
        const OpenSim::Component* c = TryGetMaybeSelectedFrameOrNullptr(uiState, model);
        return c ? c->getName() : std::string{c_OriginalFrameLabel};
    }

    void DrawOriginalFrameSelectable(FrameSelectorUiState& uiState)
    {
        const bool selected = uiState.maybeSelectedFrameAbsPath != std::nullopt;
        if (ui::draw_selectable(c_OriginalFrameLabel, selected))
        {
            uiState.maybeSelectedFrameAbsPath.reset();
        }
    }

    void DrawModelFrameSelectable(
        FrameSelectorUiState& uiState,
        const OpenSim::Frame& frame)
    {
        const std::string absPath = GetAbsolutePathString(frame);
        const bool selected = uiState.maybeSelectedFrameAbsPath == absPath;

        if (ui::draw_selectable(frame.getName(), selected))
        {
            uiState.maybeSelectedFrameAbsPath = absPath;
        }
    }

    void DrawModelFrameSelectables(
        FrameSelectorUiState& uiState,
        const OpenSim::Model& model)
    {
        int imguiID = 0;
        for (const OpenSim::Frame& frame : model.getComponentList<OpenSim::Frame>())
        {
            ui::push_id(imguiID++);
            DrawModelFrameSelectable(uiState, frame);
            ui::pop_id();
        }
    }

    void DrawFrameSelector(FrameSelectorUiState& uiState, const OpenSim::Model& model)
    {
        const std::string label = CalcComboLabel(uiState, model);
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

    std::optional<SimTK::Transform> TryGetTransformToReexpressPointsIn(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::optional<std::string>& maybeAbsPathOfFrameToReexpressPointsIn)
    {
        if (!maybeAbsPathOfFrameToReexpressPointsIn)
        {
            return std::nullopt;  // caller doesn't want re-expression
        }

        const auto* const frame = FindComponent<OpenSim::Frame>(model, *maybeAbsPathOfFrameToReexpressPointsIn);
        if (!frame)
        {
            return std::nullopt;  // the selected frame doesn't exist in the model (bug?)
        }

        return frame->getTransformInGround(state).invert();
    }

    std::vector<std::string> GetSortedListOfOutputPointAbsPaths(
        const std::unordered_set<std::string>& unorderedPointAbsPaths,
        bool shouldExportPointsWithAbsPathNames)
    {
        std::vector<std::string> rv(unorderedPointAbsPaths.begin(), unorderedPointAbsPaths.end());
        if (shouldExportPointsWithAbsPathNames) {
            rgs::sort(rv);
        }
        else {
            rgs::sort(rv, rgs::less{}, [](const auto& path) { return substring_after_last(path, '/'); });
        }
        return rv;
    }

    Vec3 CalcReexpressedFrame(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const PointInfo& pointInfo,
        const SimTK::Transform& ground2otherFrame)
    {
        const auto* const frame = FindComponent<OpenSim::Frame>(model, pointInfo.frameAbsPath);
        if (!frame)
        {
            return pointInfo.location;  // cannot find frame (bug?)
        }

        return to<Vec3>(ground2otherFrame * frame->getTransformInGround(state) * to<SimTK::Vec3>(pointInfo.location));
    }

    void TryWriteOneCSVDataRow(
        const OpenSim::Model& model,
        const SimTK::State& state,
        bool shouldExportPointsWithAbsPathNames,
        const std::optional<SimTK::Transform>& maybeGround2ReexpressedFrame,
        const std::string& pointAbsPath,
        std::ostream& out)
    {
        const OpenSim::Component* const c = FindComponent(model, pointAbsPath);
        if (!c)
        {
            return;  // skip writing: point no longer exists in model
        }

        const std::optional<PointInfo> poi = TryExtractPointInfo(*c, state);
        if (!poi)
        {
            return;  // skip writing: cannot extract point info for the component
        }

        // else: compute position, name, etc. and emit as a CSV data row

        const Vec3 position = maybeGround2ReexpressedFrame ?
            CalcReexpressedFrame(model, state, *poi, *maybeGround2ReexpressedFrame) :
            poi->location;

        const std::string name = shouldExportPointsWithAbsPathNames ?
            GetAbsolutePathString(*c) :
            c->getName();

        const auto columns = std::to_array<std::string>({
            name,
            std::to_string(position.x),
            std::to_string(position.y),
            std::to_string(position.z),
        });

        write_csv_row(out, columns);
    }

    void WritePointsAsCSVTo(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::unordered_set<std::string>& pointAbsPaths,
        const std::optional<std::string>& maybeAbsPathOfFrameToReexpressPointsIn,
        bool shouldExportPointsWithAbsPathNames,
        std::ostream& out)
    {
        const std::vector<std::string> sortedRowAbsPaths = GetSortedListOfOutputPointAbsPaths(
            pointAbsPaths,
            shouldExportPointsWithAbsPathNames
        );

        const std::optional<SimTK::Transform> maybeGround2ReexpressedFrame = TryGetTransformToReexpressPointsIn(
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
        for (const std::string& path : sortedRowAbsPaths)
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

    void ActionPromptUserForSaveLocationAndExportPoints(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const std::unordered_set<std::string>& pointAbsPaths,
        const std::optional<std::string>& maybeAbsPathOfFrameToReexpressPointsIn,
        bool shouldExportPointsWithAbsPathNames)
    {
        // Pre-write the CSV in memory so that the asynchronous user prompt doesn't depend
        // on a bunch of UI state.
        std::stringstream ss;
        WritePointsAsCSVTo(
            model,
            state,
            pointAbsPaths,
            maybeAbsPathOfFrameToReexpressPointsIn,
            shouldExportPointsWithAbsPathNames,
            ss
        );

        // Asynchronously prompt the user for a save location and write the CSV to it.
        App::upd().prompt_user_to_save_file_with_specific_extension([csv = std::move(ss).str()](std::filesystem::path p)
        {
            std::ofstream ofs;
            if (not ofs) {
                log_error("%s: error opening file for writing", p.string().c_str());
                return;
            }
            ofs << csv;
        }, "csv");
    }
}

class osc::ExportPointsPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ExportPointsPopup& owner,
        Widget* parent,
        std::string_view popupName_,
        std::shared_ptr<const IModelStatePair> model_) :

        PopupPrivate{owner, parent, popupName_},
        m_Model{std::move(model_)}
    {}

    void draw_content()
    {
        const OpenSim::Model& model = m_Model->getModel();
        const SimTK::State& state = m_Model->getState();

        const float sectionSpacing = 0.5f*ui::get_text_line_height();

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

private:
    void drawBottomButtons()
    {
        if (ui::draw_button("Cancel"))
        {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button(OSC_ICON_UPLOAD " Export to CSV"))
        {
            ActionPromptUserForSaveLocationAndExportPoints(
                m_Model->getModel(),
                m_Model->getState(),
                m_PointSelectorState.selectedPointAbsPaths,
                m_FrameSelectorState.maybeSelectedFrameAbsPath,
                m_OutputFormatState.exportPointNamesAsAbsPaths
            );
        }
    }

    std::shared_ptr<const IModelStatePair> m_Model;
    PointSelectorUiState m_PointSelectorState;
    FrameSelectorUiState m_FrameSelectorState;
    OutputFormatEditorUiState m_OutputFormatState;
};


osc::ExportPointsPopup::ExportPointsPopup(
    Widget* parent,
    std::string_view popupName,
    std::shared_ptr<const IModelStatePair> model_) :

    Popup{std::make_unique<Impl>(*this, parent, popupName, std::move(model_))}
{}
void osc::ExportPointsPopup::impl_draw_content() { private_data().draw_content(); }
