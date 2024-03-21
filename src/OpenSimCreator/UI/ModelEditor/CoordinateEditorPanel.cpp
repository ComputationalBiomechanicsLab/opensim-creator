#include "CoordinateEditorPanel.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/ComponentContextMenu.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>

#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;

class osc::CoordinateEditorPanel::Impl final : public StandardPanelImpl {
public:

    Impl(
        std::string_view panelName_,
        ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> uum_) :

        StandardPanelImpl{panelName_},
        m_MainUIStateAPI{mainUIStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(uum_)}
    {
    }

private:

    void implDrawContent() final
    {
        // load coords
        std::vector<OpenSim::Coordinate const*> coordPtrs = GetCoordinatesInModel(m_Model->getModel());

        // if there's no coordinates in the model, show a warning message and stop drawing
        if (coordPtrs.empty())
        {
            CStringView const msg = "(there are no coordinates in the model)";
            float const w = ui::CalcTextSize(msg).x;
            ui::SetCursorPosX(0.5f * (ui::GetContentRegionAvail().x - w));  // center align
            ui::TextDisabled(msg);
            return;
        }

        // else: there's coordinates, which should be shown in a table
        ImGuiTableFlags flags =
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Sortable |
            ImGuiTableFlags_SortTristate |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingStretchSame;
        if (ui::BeginTable("##coordinatestable", 3, flags))
        {
            ui::TableSetupColumn("Name");
            ui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoSort, 1.65f);
            ui::TableSetupColumn("Speed", ImGuiTableColumnFlags_NoSort, 0.5f);
            ui::TableSetupScrollFreeze(0, 1);
            ui::TableHeadersRow();

            if (ImGuiTableSortSpecs* p = ui::TableGetSortSpecs(); p && p->SpecsDirty)
            {
                std::span<ImGuiTableColumnSortSpecs const> specs(p->Specs, p->SpecsCount);

                // we know the user can only sort one column (name) so we don't need to permute
                // through the entire specs structure
                if (specs.size() == 1 && specs.front().ColumnIndex == 0 && specs.front().SortOrder == 0)
                {
                    switch (specs.front().SortDirection)
                    {
                    case ImGuiSortDirection_Ascending:
                        std::sort(
                            coordPtrs.begin(),
                            coordPtrs.end(),
                            IsNameLexographicallyLowerThan<OpenSim::Component const*>
                        );
                        break;
                    case ImGuiSortDirection_Descending:
                        std::sort(
                            coordPtrs.begin(),
                            coordPtrs.end(),
                            IsNameLexographicallyGreaterThan<OpenSim::Component const*>
                        );
                        break;
                    case ImGuiSortDirection_None:
                    default:
                        break;  // just use them as-is
                    }
                }
            }

            int id = 0;
            for (OpenSim::Coordinate const* coordPtr : coordPtrs)
            {
                ui::PushID(id++);
                drawRow(*coordPtr);
                ui::PopID();
            }

            ui::EndTable();
        }
    }

    void drawRow(OpenSim::Coordinate const& c)
    {
        ui::TableNextRow();

        int column = 0;
        ui::TableSetColumnIndex(column++);
        drawNameCell(c);
        ui::TableSetColumnIndex(column++);
        drawDataCell(c);
        ui::TableSetColumnIndex(column++);
        drawSpeedCell(c);
    }

    void drawNameCell(OpenSim::Coordinate const& c)
    {
        int stylesPushed = 0;
        if (&c == m_Model->getHovered())
        {
            ui::PushStyleColor(ImGuiCol_Text, Color::yellow());
            ++stylesPushed;
        }
        if (&c == m_Model->getSelected())
        {
            ui::PushStyleColor(ImGuiCol_Text, Color::yellow());
            ++stylesPushed;
        }

        ui::TextUnformatted(c.getName());
        ui::PopStyleColor(std::exchange(stylesPushed, 0));

        if (ui::IsItemHovered())
        {
            m_Model->setHovered(&c);

            std::stringstream ss;
            ss << "    motion type = " << GetMotionTypeDisplayName(c) << '\n';
            ss << "    owner = " << TryGetOwnerName(c).value_or("(no owner)");

            ui::DrawTooltip(c.getName(), ss.str());
        }

        if (ui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_Model->setSelected(&c);
        }
        else if (ui::IsItemClicked(ImGuiMouseButton_Right))
        {
            auto popup = std::make_unique<ComponentContextMenu>(
                "##componentcontextmenu",
                m_MainUIStateAPI,
                m_EditorAPI,
                m_Model,
                GetAbsolutePath(c)
            );
            popup->open();
            m_EditorAPI->pushPopup(std::move(popup));
        }
    }

    void drawDataCell(OpenSim::Coordinate const& c)
    {
        drawDataCellLockButton(c);
        ui::SameLine(0.0f, 0.0f);
        drawDataCellCoordinateSlider(c);
    }

    void drawDataCellLockButton(OpenSim::Coordinate const& c)
    {
        ui::PushStyleColor(ImGuiCol_Button, Color::clear());
        ui::PushStyleColor(ImGuiCol_ButtonActive, Color::clear());
        ui::PushStyleColor(ImGuiCol_ButtonHovered, Color::clear());
        ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ui::GetStyleFramePadding().y});
        if (ui::Button(c.getLocked(m_Model->getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK))
        {
            bool newValue = !c.getLocked(m_Model->getState());
            ActionSetCoordinateLockedAndSave(*m_Model, c, newValue);
        }
        ui::PopStyleVar();
        ui::PopStyleColor();
        ui::PopStyleColor();
        ui::PopStyleColor();
        ui::DrawTooltipIfItemHovered("Toggle Coordinate Lock", "Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");
    }

    void drawDataCellCoordinateSlider(OpenSim::Coordinate const& c)
    {
        bool const coordinateLocked = c.getLocked(m_Model->getState());

        ui::SetNextItemWidth(ui::GetContentRegionAvail().x);

        float minValue = ConvertCoordValueToDisplayValue(c, c.getRangeMin());
        float maxValue = ConvertCoordValueToDisplayValue(c, c.getRangeMax());
        float displayedValue = ConvertCoordValueToDisplayValue(c, c.getValue(m_Model->getState()));

        if (coordinateLocked)
        {
            ui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.2f);
            ui::BeginDisabled();
        }
        if (ui::CircularSliderFloat("##coordinatevalueeditor", &displayedValue, minValue, maxValue))
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValue(*m_Model, c, storedValue);
        }
        if (coordinateLocked)
        {
            ui::EndDisabled();
            ui::PopStyleVar();
        }
        if (ui::IsItemDeactivatedAfterEdit())
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValueAndSave(*m_Model, c, storedValue);
        }
        ui::DrawTooltipBodyOnlyIfItemHovered("Ctrl-click the slider to edit");
    }

    void drawSpeedCell(OpenSim::Coordinate const& c)
    {
        float displayedSpeed = ConvertCoordValueToDisplayValue(c, c.getSpeedValue(m_Model->getState()));

        ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
        if (ui::InputMetersFloat("##coordinatespeededitor", displayedSpeed))
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            ActionSetCoordinateSpeed(*m_Model, c, storedSpeed);
        }

        if (ui::IsItemDeactivatedAfterEdit())
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            ActionSetCoordinateSpeedAndSave(*m_Model, c, storedSpeed);
        }
    }

    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};


// public API

osc::CoordinateEditorPanel::CoordinateEditorPanel(
    std::string_view panelName_,
    ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
    IEditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> uum_) :

    m_Impl{std::make_unique<Impl>(panelName_, mainUIStateAPI_, editorAPI_, std::move(uum_))}
{
}

osc::CoordinateEditorPanel::CoordinateEditorPanel(CoordinateEditorPanel&&) noexcept = default;
osc::CoordinateEditorPanel& osc::CoordinateEditorPanel::operator=(CoordinateEditorPanel&&) noexcept = default;
osc::CoordinateEditorPanel::~CoordinateEditorPanel() noexcept = default;

CStringView osc::CoordinateEditorPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::CoordinateEditorPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::CoordinateEditorPanel::implOpen()
{
    m_Impl->open();
}

void osc::CoordinateEditorPanel::implClose()
{
    m_Impl->close();
}

void osc::CoordinateEditorPanel::implOnDraw()
{
    m_Impl->onDraw();
}
