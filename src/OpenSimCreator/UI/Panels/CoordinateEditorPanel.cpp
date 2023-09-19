#include "CoordinateEditorPanel.hpp"

#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/UI/Middleware/EditorAPI.hpp>
#include <OpenSimCreator/UI/Widgets/ComponentContextMenu.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <OpenSimCreator/Utils/UndoableModelActions.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/UI/Panels/StandardPanel.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <string>
#include <string_view>
#include <sstream>
#include <utility>
#include <vector>

class osc::CoordinateEditorPanel::Impl final : public osc::StandardPanel {
public:

    Impl(
        std::string_view panelName_,
        ParentPtr<MainUIStateAPI> const& mainUIStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> uum_) :

        StandardPanel{panelName_},
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
            float const w = ImGui::CalcTextSize(msg.c_str()).x;
            ImGui::SetCursorPosX(0.5f * (ImGui::GetContentRegionAvail().x - w));  // center align
            ImGui::TextDisabled("%s", msg.c_str());
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
        if (ImGui::BeginTable("##coordinatestable", 3, flags))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoSort, 1.65f);
            ImGui::TableSetupColumn("Speed", ImGuiTableColumnFlags_NoSort, 0.5f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* p = ImGui::TableGetSortSpecs(); p && p->SpecsDirty)
            {
                nonstd::span<ImGuiTableColumnSortSpecs const> specs(p->Specs, p->SpecsCount);

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
                            osc::IsNameLexographicallyLowerThan<OpenSim::Component const*>
                        );
                        break;
                    case ImGuiSortDirection_Descending:
                        std::sort(
                            coordPtrs.begin(),
                            coordPtrs.end(),
                            osc::IsNameLexographicallyGreaterThan<OpenSim::Component const*>
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
                ImGui::PushID(id++);
                drawRow(*coordPtr);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void drawRow(OpenSim::Coordinate const& c)
    {
        ImGui::TableNextRow();

        int column = 0;
        ImGui::TableSetColumnIndex(column++);
        drawNameCell(c);
        ImGui::TableSetColumnIndex(column++);
        drawDataCell(c);
        ImGui::TableSetColumnIndex(column++);
        drawSpeedCell(c);
    }

    void drawNameCell(OpenSim::Coordinate const& c)
    {
        int stylesPushed = 0;
        if (&c == m_Model->getHovered())
        {
            osc::PushStyleColor(ImGuiCol_Text, osc::Color::yellow());
            ++stylesPushed;
        }
        if (&c == m_Model->getSelected())
        {
            osc::PushStyleColor(ImGuiCol_Text, osc::Color::yellow());
            ++stylesPushed;
        }

        ImGui::TextUnformatted(c.getName().c_str());
        ImGui::PopStyleColor(std::exchange(stylesPushed, 0));

        if (ImGui::IsItemHovered())
        {
            m_Model->setHovered(&c);

            std::stringstream ss;
            ss << "    motion type = " << osc::GetMotionTypeDisplayName(c) << '\n';
            ss << "    owner = " << osc::TryGetOwnerName(c).value_or("(no owner)");

            osc::DrawTooltip(c.getName(), ss.str());
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_Model->setSelected(&c);
        }
        else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            auto popup = std::make_unique<ComponentContextMenu>(
                "##componentcontextmenu",
                m_MainUIStateAPI,
                m_EditorAPI,
                m_Model,
                osc::GetAbsolutePath(c)
            );
            popup->open();
            m_EditorAPI->pushPopup(std::move(popup));
        }
    }

    void drawDataCell(OpenSim::Coordinate const& c)
    {
        drawDataCellLockButton(c);
        ImGui::SameLine(0.0f, 0.0f);
        drawDataCellCoordinateSlider(c);
    }

    void drawDataCellLockButton(OpenSim::Coordinate const& c)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ImGui::GetStyle().FramePadding.y});
        if (ImGui::Button(c.getLocked(m_Model->getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK))
        {
            bool newValue = !c.getLocked(m_Model->getState());
            ActionSetCoordinateLockedAndSave(*m_Model, c, newValue);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        osc::DrawTooltipIfItemHovered("Toggle Coordinate Lock", "Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");
    }

    void drawDataCellCoordinateSlider(OpenSim::Coordinate const& c)
    {
        bool const coordinateLocked = c.getLocked(m_Model->getState());

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        float minValue = ConvertCoordValueToDisplayValue(c, c.getRangeMin());
        float maxValue = ConvertCoordValueToDisplayValue(c, c.getRangeMax());
        float displayedValue = ConvertCoordValueToDisplayValue(c, c.getValue(m_Model->getState()));

        if (coordinateLocked)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.2f);
            ImGui::BeginDisabled();
        }
        if (CircularSliderFloat("##coordinatevalueeditor", &displayedValue, minValue, maxValue))
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValue(*m_Model, c, storedValue);
        }
        if (coordinateLocked)
        {
            ImGui::EndDisabled();
            ImGui::PopStyleVar();
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValueAndSave(*m_Model, c, storedValue);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Ctrl-click the slider to edit");
    }

    void drawSpeedCell(OpenSim::Coordinate const& c)
    {
        float displayedSpeed = ConvertCoordValueToDisplayValue(c, c.getSpeedValue(m_Model->getState()));

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (InputMetersFloat("##coordinatespeededitor", displayedSpeed))
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            osc::ActionSetCoordinateSpeed(*m_Model, c, storedSpeed);
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            osc::ActionSetCoordinateSpeedAndSave(*m_Model, c, storedSpeed);
        }
    }

    ParentPtr<MainUIStateAPI> m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};


// public API

osc::CoordinateEditorPanel::CoordinateEditorPanel(
    std::string_view panelName_,
    ParentPtr<MainUIStateAPI> const& mainUIStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> uum_) :

    m_Impl{std::make_unique<Impl>(panelName_, mainUIStateAPI_, editorAPI_, std::move(uum_))}
{
}

osc::CoordinateEditorPanel::CoordinateEditorPanel(CoordinateEditorPanel&&) noexcept = default;
osc::CoordinateEditorPanel& osc::CoordinateEditorPanel::operator=(CoordinateEditorPanel&&) noexcept = default;
osc::CoordinateEditorPanel::~CoordinateEditorPanel() noexcept = default;

osc::CStringView osc::CoordinateEditorPanel::implGetName() const
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
