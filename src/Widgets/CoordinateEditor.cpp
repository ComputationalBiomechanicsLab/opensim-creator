#include "CoordinateEditor.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <vector>


// returns `true` if the name of `c1` is lexographically less than `c2`
static bool IsNameLexographicallyLessThan(OpenSim::Coordinate const* c1,
                                          OpenSim::Coordinate const* c2)
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
        ImGui::TextUnformatted(ICON_FA_EYE);

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Right-click for filtering options");
            ImGui::EndTooltip();
        }

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
        osc::InputString("##coords search filter", m_Filter, m_FilterMaxLen);
        ImGui::Dummy({0.0f, 3.0f});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        // load coords
        m_CoordScratch.clear();
        GetCoordinatesInModel(m_Uum->getModel(), m_CoordScratch);

        // sort coords
        RemoveErase(m_CoordScratch, [this](auto const* c)
        {
            return shouldFilterOut(*c);
        });

        // sort coords
        if (m_SortByName)
        {
            Sort(m_CoordScratch, IsNameLexographicallyLessThan);
        }

        // "save to model" - save current coordinate values to the model
        if (ImGui::Button(ICON_FA_SAVE " Save to Model"))
        {
            osc::ActionSaveCoordinateEditsToModel(*m_Uum);
        }
        osc::DrawTooltipIfItemHovered("Save Coordinate Edits to Model", "Saves the current set of coordinate edits to the model file as default values. This ensures that the current set of coordinate edits are saved in the resulting osim file, and that those edits will be shown when the osim is initially loaded.");

        ImGui::Dummy({0.0f, 0.5f * ImGui::GetTextLineHeight()});

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

        // draw (lack of) coordinates
        if (m_CoordScratch.empty())
        {
            ImGui::Columns();
            ImGui::NewLine();
            ImGui::TextDisabled("    (no coordinates in this model)");
            ImGui::Columns(3);
        }

        int i = 0;

        for (OpenSim::Coordinate const* c : m_CoordScratch)
        {
            ImGui::PushID(i++);

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
            ImGui::Text("%s", c->getName().c_str());
            ImGui::PopStyleColor(std::exchange(stylesPushed, 0));

            if (ImGui::IsItemHovered())
            {
                m_Uum->setHovered(c);

                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
                char const* type = "Unknown";
                switch (c->getMotionType()) {
                case OpenSim::Coordinate::MotionType::Rotational:
                    type = "Rotational";
                    break;
                case OpenSim::Coordinate::MotionType::Translational:
                    type = "Translational";
                    break;
                case OpenSim::Coordinate::MotionType::Coupled:
                    type = "Coupled";
                    break;
                default:
                    type = "Unknown";
                }

                ImGui::Text("%s Coordinate, Owner = %s", type, c->hasOwner() ? c->getOwner().getName().c_str() : "(no owner)");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) || ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                m_Uum->setSelected(c);
            }

            ImGui::NextColumn();

            if (c->getLocked(m_Uum->getState()))
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
                ++stylesPushed;
            }

            if (ImGui::Button(c->getLocked(m_Uum->getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK))
            {
                m_Uum->updUiModel().pushCoordinateEdit(*c, CoordinateEdit{c->getValue(m_Uum->getState()), c->getSpeedValue(m_Uum->getState()), !c->getLocked(m_Uum->getState())});
                m_Uum->commit("(un)locked coordinate");
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
                ImGui::Text("Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::SameLine();

            float v = ConvertCoordValueToDisplayValue(*c, c->getValue(m_Uum->getState()));
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##coordinatevalueeditor", &v, ConvertCoordValueToDisplayValue(*c, c->getRangeMin()), ConvertCoordValueToDisplayValue(*c, c->getRangeMax())))
            {
                m_Uum->updUiModel().pushCoordinateEdit(*c, CoordinateEdit{
                    ConvertCoordDisplayValueToStorageValue(*c, v),
                    c->getSpeedValue(m_Uum->getState()),
                    c->getLocked(m_Uum->getState())
                });
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
                    m_Uum->updUiModel().removeCoordinateEdit(*c);
                    m_Uum->commit("reset coordinate");
                }
                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(stylesPushed);
            stylesPushed = 0;
            ImGui::NextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

            float speed = ConvertCoordValueToDisplayValue(*c, c->getSpeedValue(m_Uum->getState()));
            if (InputMetersFloat("##coordinatespeededitor", &speed))
            {
                m_Uum->updUiModel().pushCoordinateEdit(*c, CoordinateEdit{
                    c->getValue(m_Uum->getState()),
                    ConvertCoordDisplayValueToStorageValue(*c, speed),
                    c->getLocked(m_Uum->getState())
                });
            }
            ImGui::NextColumn();

            ImGui::PopID();
        }
        ImGui::Columns();

        ImGui::EndChild();
    }

private:

    bool shouldFilterOut(OpenSim::Coordinate const& c)
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
    static constexpr inline int m_FilterMaxLen = 64;
    bool m_SortByName = false;
    bool m_ShowRotational = true;
    bool m_ShowTranslational = true;
    bool m_ShowCoupled = true;
    std::vector<OpenSim::Coordinate const*> m_CoordScratch;
};

osc::CoordinateEditor::CoordinateEditor(std::shared_ptr<UndoableModelStatePair> uum) :
    m_Impl{std::make_unique<Impl>(std::move(uum))}
{
}
osc::CoordinateEditor::CoordinateEditor(CoordinateEditor&&) noexcept = default;
osc::CoordinateEditor& osc::CoordinateEditor::operator=(CoordinateEditor&&) noexcept = default;
osc::CoordinateEditor::~CoordinateEditor() noexcept = default;

void osc::CoordinateEditor::draw()
{
    m_Impl->draw();
}
