#include "CoordinateEditor.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <sstream>
#include <vector>


static constexpr inline int g_FilterMaxLen = 64;


// returns `true` if the name of `c1` is lexographically less than `c2`
static bool IsNameLexographicallyLessThan(OpenSim::Component const* c1, OpenSim::Component const* c2)
{
    return c1->getName() < c2->getName();
}

class osc::CoordinateEditor::Impl final {
public:
    Impl(std::shared_ptr<UndoableModelStatePair> uum) :
        m_Uum{std::move(uum)}
    {
    }

    void draw()
    {
        ImGui::Dummy({0.0f, 3.0f});

        drawTopBar();

        ImGui::Dummy({0.0f, 3.0f});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        drawSaveCoordsButton();

        ImGui::Dummy({0.0f, 0.5f * ImGui::GetTextLineHeight()});

        drawCoordinatesTable();
    }

private:

    void drawTopBar()
    {
        ImGui::TextUnformatted(ICON_FA_EYE);
        osc::DrawTooltipIfItemHovered("Filter Coordinates", "Right-click for filtering options");

        // draw filter popup (checkboxes for editing filters/sort etc)
        if (ImGui::BeginPopupContextItem("##coordinateditorfilterpopup"))
        {
            ImGui::Checkbox("sort alphabetically", &m_SortByName);
            ImGui::Checkbox("show rotational coords", &m_ShowRotational);
            ImGui::Checkbox("show translational coords", &m_ShowTranslational);
            ImGui::Checkbox("show coupled coords", &m_ShowCoupled);
            ImGui::EndPopup();
        }

        // draw "clear search" button
        ImGui::SameLine();
        if (!m_Filter.empty())
        {
            if (ImGui::Button("X"))
            {
                m_Filter.clear();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Clear the search string");
                ImGui::EndTooltip();
            }
        }
        else
        {
            ImGui::TextUnformatted(ICON_FA_SEARCH);
        }

        // draw search bar
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        osc::InputString("##coords search filter", m_Filter, g_FilterMaxLen);
    }

    void drawSaveCoordsButton()
    {
        // "save to model" - save current coordinate values to the model
        if (ImGui::Button(ICON_FA_SAVE " Save to Model"))
        {
            osc::ActionSaveCoordinateEditsToModel(*m_Uum);
        }
        osc::DrawTooltipIfItemHovered("Save Coordinate Edits to Model", "Saves the current set of coordinate edits to the model file as default values. This ensures that the current set of coordinate edits are saved in the resulting osim file, and that those edits will be shown when the osim is initially loaded.");
    }

    void drawCoordinatesTable()
    {
        // load coords
        std::vector<OpenSim::Coordinate const*> coordPtrs = getShownCoordinates();

        // draw header
        ImGui::Columns(3);
        ImGui::Text("Coordinate");
        ImGui::SameLine();
        DrawHelpMarker("Name of the coordinate.\n\nIn OpenSim, coordinates typically parameterize joints. Different joints have different coordinates. For example, a PinJoint has one rotational coordinate, a FreeJoint has 6 coordinates (3 translational, 3 rotational), a WeldJoint has no coordinates. This list shows all the coordinates in the model.");
        ImGui::NextColumn();
        ImGui::Text("Value");
        ImGui::SameLine();
        DrawHelpMarker("Initial value of the coordinate.\n\nThis sets the initial value of a coordinate in the first state of the simulation. You can `Ctrl+Click` sliders when you want to type a value in.");
        ImGui::NextColumn();
        ImGui::Text("Speed");
        ImGui::SameLine();
        DrawHelpMarker("Initial speed of the coordinate.\n\nThis sets the 'velocity' of the coordinate in the first state of the simulation. It enables you to (e.g.) start a simulation with something moving in the model.");
        ImGui::NextColumn();

        // draw separator between header and coordinates
        ImGui::Columns();
        ImGui::Separator();
        ImGui::Columns(3);

        if (coordPtrs.empty())
        {
            // draw (lack of) coordinates
            ImGui::Columns();
            ImGui::NewLine();
            ImGui::TextDisabled("    (no coordinates in this model)");
            ImGui::Columns(3);
        }
        else
        {
            // draw each coordinate row
            int i = 0;
            for (OpenSim::Coordinate const* c : coordPtrs)
            {
                ImGui::PushID(i++);
                drawRow(c);
                ImGui::PopID();
            }
        }

        ImGui::Columns();
    }

    std::vector<OpenSim::Coordinate const*> getShownCoordinates() const
    {
        std::vector<OpenSim::Coordinate const*> rv;

        GetCoordinatesInModel(m_Uum->getModel(), rv);

        // sort coords
        RemoveErase(rv, [this](auto const* c)
        {
            return shouldFilterOut(*c);
        });

        // sort coords
        if (m_SortByName)
        {
            Sort(rv, IsNameLexographicallyLessThan);
        }

        return rv;
    }

    void drawRow(OpenSim::Coordinate const* c)
    {
        drawNameCell(c);

        ImGui::NextColumn();

        drawDataCell(c);

        ImGui::NextColumn();

        drawSpeedCell(c);

        ImGui::NextColumn();
    }

    void drawNameCell(OpenSim::Coordinate const* c)
    {
        int stylesPushed = 0;
        if (c == m_Uum->getHovered())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
            ++stylesPushed;
        }
        if (c == m_Uum->getSelected())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
            ++stylesPushed;
        }

        ImGui::TextUnformatted(c->getName().c_str());
        ImGui::PopStyleColor(std::exchange(stylesPushed, 0));

        if (ImGui::IsItemHovered())
        {
            m_Uum->setHovered(c);

            char const* motionType = osc::GetMotionTypeDisplayName(*c);

            std::stringstream ss;
            ss << "    motion type = " << osc::GetMotionTypeDisplayName(*c) << '\n';
            ss << "    owner = " << (c->hasOwner() ? c->getOwner().getName().c_str() : "(no owner)");

            osc::DrawTooltip(c->getName().c_str(), ss.str().c_str());
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) || ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_Uum->setSelected(c);
        }
    }

    void drawDataCell(OpenSim::Coordinate const* c)
    {
        int stylesPushed = 0;

        if (c->getLocked(m_Uum->getState()))
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
            ++stylesPushed;
        }

        if (ImGui::Button(c->getLocked(m_Uum->getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK))
        {
            bool newValue = !c->getLocked(m_Uum->getState());
            ActionSetCoordinateLocked(*m_Uum, *c, newValue);
        }
        osc::DrawTooltipIfItemHovered("Toggle Coordinate Lock", "Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

        float minValue = ConvertCoordValueToDisplayValue(*c, c->getRangeMin());
        float maxValue = ConvertCoordValueToDisplayValue(*c, c->getRangeMax());
        float displayedValue = ConvertCoordValueToDisplayValue(*c, c->getValue(m_Uum->getState()));
        if (ImGui::SliderFloat("##coordinatevalueeditor", &displayedValue, minValue, maxValue))
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(*c, displayedValue);
            ActionSetCoordinateValue(*m_Uum, *c, storedValue);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(*c, displayedValue);
            ActionSetCoordinateValueAndSave(*m_Uum, *c, storedValue);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Ctrl-click the slider to edit");
            ImGui::EndTooltip();
        }

        // draw filter popup (checkboxes for editing filters/sort etc)
        if (ImGui::BeginPopupContextItem("##coordinatecontextmenu"))
        {
            if (ImGui::MenuItem("reset"))
            {
                ActionWipeCoordinateEdits(*m_Uum, *c);
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(stylesPushed);
    }

    void drawSpeedCell(OpenSim::Coordinate const* c)
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

        float displayedSpeed = ConvertCoordValueToDisplayValue(*c, c->getSpeedValue(m_Uum->getState()));

        if (InputMetersFloat("##coordinatespeededitor", &displayedSpeed))
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(*c, displayedSpeed);
            osc::ActionSetCoordinateSpeed(*m_Uum, *c, displayedSpeed);
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(*c, displayedSpeed);
            osc::ActionSetCoordinateSpeedAndSave(*m_Uum, *c, displayedSpeed);
        }
    }

    bool shouldFilterOut(OpenSim::Coordinate const& c) const
    {
        if (!osc::ContainsSubstringCaseInsensitive(c.getName(), m_Filter))
        {
            return true;
        }

        OpenSim::Coordinate::MotionType mt = c.getMotionType();

        if (m_ShowRotational && mt == OpenSim::Coordinate::MotionType::Rotational)
        {
            return false;
        }
        else if (m_ShowTranslational && mt == OpenSim::Coordinate::MotionType::Translational)
        {
            return false;
        }
        else if (m_ShowCoupled && mt == OpenSim::Coordinate::MotionType::Coupled)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    std::shared_ptr<UndoableModelStatePair> m_Uum;
    std::string m_Filter;
    bool m_SortByName = false;
    bool m_ShowRotational = true;
    bool m_ShowTranslational = true;
    bool m_ShowCoupled = true;
};


// public API

osc::CoordinateEditor::CoordinateEditor(std::shared_ptr<UndoableModelStatePair> uum) :
    m_Impl{new Impl{std::move(uum)}}
{
}

osc::CoordinateEditor::CoordinateEditor(CoordinateEditor&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::CoordinateEditor& osc::CoordinateEditor::operator=(CoordinateEditor&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::CoordinateEditor::~CoordinateEditor() noexcept
{
    delete m_Impl;
}

void osc::CoordinateEditor::draw()
{
    m_Impl->draw();
}
