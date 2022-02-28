#include "CoordinateEditor.hpp"

#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Styling.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <imgui.h>


#include "src/OpenSimBindings/UiModel.hpp"

#include <vector>


// returns `true` if the name of `c1` is lexographically less than `c2`
static bool IsNameLexographicallyLessThan(OpenSim::Coordinate const* c1,
                                          OpenSim::Coordinate const* c2)
{
    return c1->getName() < c2->getName();
}

static float ConvertToDisplayFormat(OpenSim::Coordinate const& c, double v)
{
    float rv = static_cast<float>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational)
    {
        rv = glm::degrees(rv);
    }

    return rv;
}

static double ConvertToStorageFormat(OpenSim::Coordinate const& c, float v)
{
    double rv = static_cast<double>(v);

    if (c.getMotionType() == OpenSim::Coordinate::MotionType::Rotational)
    {
        rv = glm::radians(rv);
    }

    return rv;
}


class osc::CoordinateEditor::Impl final {
public:
    char filter[64]{};
    bool sort_by_name = false;
    bool show_rotational = true;
    bool show_translational = true;
    bool show_coupled = true;
    std::vector<OpenSim::Coordinate const*> coord_scratch;

    bool draw(UiModel& uim)
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
            ImGui::Checkbox("sort alphabetically", &sort_by_name);
            ImGui::Checkbox("show rotational coords", &show_rotational);
            ImGui::Checkbox("show translational coords", &show_translational);
            ImGui::Checkbox("show coupled coords", &show_coupled);
            ImGui::EndPopup();
        }

        // draw "clear search" button
        ImGui::SameLine();
        if (filter[0] != '\0')
        {
            if (ImGui::Button("X"))
            {
                filter[0] = '\0';
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
        ImGui::InputText("##coords search filter", filter, sizeof(filter));
        ImGui::Dummy({0.0f, 3.0f});
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});

        // load coords
        coord_scratch.clear();
        GetCoordinatesInModel(uim.getModel(), coord_scratch);

        // sort coords
        RemoveErase(coord_scratch, [this](auto const* c)
        {
            return shouldFilterOut(*c);
        });

        // sort coords
        if (sort_by_name)
        {
            Sort(coord_scratch, IsNameLexographicallyLessThan);
        }

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
        if (coord_scratch.empty())
        {
            ImGui::Columns();
            ImGui::NewLine();
            ImGui::TextDisabled("    (no coordinates in this model)");
            ImGui::Columns(3);
        }

        int i = 0;
        bool state_modified = false;

        for (OpenSim::Coordinate const* c : coord_scratch)
        {
            ImGui::PushID(i++);

            int styles_pushed = 0;
            if (c == uim.getHovered())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
                ++styles_pushed;
            }
            if (c == uim.getSelected())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
                ++styles_pushed;
            }
            ImGui::Text("%s", c->getName().c_str());
            ImGui::PopStyleColor(styles_pushed);
            styles_pushed = 0;

            if (ImGui::IsItemHovered())
            {
                uim.setHovered(c);

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
                uim.setSelected(c);
            }

            ImGui::NextColumn();

            if (c->getLocked(uim.getState()))
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
                ++styles_pushed;
            }

            if (ImGui::Button(c->getLocked(uim.getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK))
            {
                uim.pushCoordinateEdit(*c, CoordinateEdit{c->getValue(uim.getState()), c->getSpeedValue(uim.getState()), !c->getLocked(uim.getState())});
                state_modified = true;
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

            float v = ConvertToDisplayFormat(*c, c->getValue(uim.getState()));
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##coordinatevalueeditor", &v, ConvertToDisplayFormat(*c, c->getRangeMin()), ConvertToDisplayFormat(*c, c->getRangeMax())))
            {
                uim.pushCoordinateEdit(*c, CoordinateEdit{
                    ConvertToStorageFormat(*c, v),
                    c->getSpeedValue(uim.getState()),
                    c->getLocked(uim.getState())
                });
                state_modified = true;
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
                    uim.removeCoordinateEdit(*c);
                }
                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(styles_pushed);
            styles_pushed = 0;
            ImGui::NextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

            float speed = ConvertToDisplayFormat(*c, c->getSpeedValue(uim.getState()));
            if (InputMetersFloat("##coordinatespeededitor", &speed))
            {
                uim.pushCoordinateEdit(*c, CoordinateEdit{
                    c->getValue(uim.getState()),
                    ConvertToStorageFormat(*c, speed),
                    c->getLocked(uim.getState())
                });
                state_modified = true;
            }
            ImGui::NextColumn();

            ImGui::PopID();
        }
        ImGui::Columns();

        ImGui::EndChild();

        return state_modified;
    }


    bool shouldFilterOut(OpenSim::Coordinate const& c)
    {
        if (!osc::ContainsSubstringCaseInsensitive(c.getName(), filter))
        {
            return true;
        }

        OpenSim::Coordinate::MotionType mt = c.getMotionType();
        if (show_rotational && mt == OpenSim::Coordinate::MotionType::Rotational)
        {
            return false;
        }

        if (show_translational && mt == OpenSim::Coordinate::MotionType::Translational)
        {
            return false;
        }

        if (show_coupled && mt == OpenSim::Coordinate::MotionType::Coupled)
        {
            return false;
        }

        return true;
    }
};

osc::CoordinateEditor::CoordinateEditor() : m_Impl{std::make_unique<Impl>()} {}
osc::CoordinateEditor::CoordinateEditor(CoordinateEditor&&) noexcept = default;
osc::CoordinateEditor& osc::CoordinateEditor::operator=(CoordinateEditor&&) noexcept = default;
osc::CoordinateEditor::~CoordinateEditor() noexcept = default;

bool osc::CoordinateEditor::draw(UiModel& uim)
{
    return m_Impl->draw(uim);
}
