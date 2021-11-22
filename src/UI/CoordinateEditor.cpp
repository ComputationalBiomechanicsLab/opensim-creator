#include "CoordinateEditor.hpp"

#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Styling.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <imgui.h>

static bool do_sort_by_name(OpenSim::Coordinate const* c1, OpenSim::Coordinate const* c2) {
    return c1->getName() < c2->getName();
}

static bool should_filter_out(osc::CoordinateEditor const& st, OpenSim::Coordinate const* c) {
    if (!osc::ContainsSubstringCaseInsensitive(c->getName(), st.filter)) {
        return true;
    }

    OpenSim::Coordinate::MotionType mt = c->getMotionType();
    if (st.show_rotational && mt == OpenSim::Coordinate::MotionType::Rotational) {
        return false;
    }

    if (st.show_translational && mt == OpenSim::Coordinate::MotionType::Translational) {
        return false;
    }

    if (st.show_coupled && mt == OpenSim::Coordinate::MotionType::Coupled) {
        return false;
    }

    return true;
}

void osc::get_coordinates(OpenSim::Model const& m, std::vector<OpenSim::Coordinate const*>& out) {
    OpenSim::CoordinateSet const& s = m.getCoordinateSet();
    int len = s.getSize();
    out.reserve(out.size() + static_cast<size_t>(len));
    for (int i = 0; i < len; ++i) {
        out.push_back(&s[i]);
    }
}

bool osc::CoordinateEditor::draw(UiModel& uim) {
    ImGui::Dummy({0.0f, 3.0f});
    ImGui::TextUnformatted(ICON_FA_EYE);
    if (ImGui::BeginPopupContextItem("##coordinateditorfilterpopup")) {
        ImGui::Checkbox("sort alphabetically", &sort_by_name);
        ImGui::Checkbox("show rotational coords", &show_rotational);
        ImGui::Checkbox("show translational coords", &show_translational);
        ImGui::Checkbox("show coupled coords", &show_coupled);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (filter[0] != '\0') {
        if (ImGui::Button("X")) {
            filter[0] = '\0';
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Clear the search string");
            ImGui::EndTooltip();
        }
    } else {
        ImGui::TextUnformatted(ICON_FA_SEARCH);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    ImGui::InputText("##coords search filter", filter, sizeof(filter));
    ImGui::Dummy({0.0f, 3.0f});
    ImGui::Separator();
    ImGui::Dummy({0.0f, 3.0f});

    // load coords
    coord_scratch.clear();
    get_coordinates(uim.getModel(), coord_scratch);

    // sort coords
    {
        auto pred = [this](auto const* c) { return should_filter_out(*this, c); };
        auto it = std::remove_if(coord_scratch.begin(), coord_scratch.end(), pred);
        coord_scratch.erase(it, coord_scratch.end());
    }

    // sort coords
    if (sort_by_name) {
        std::sort(coord_scratch.begin(), coord_scratch.end(), do_sort_by_name);
    }

    ImGui::BeginChild("##coordinatesliderschildheaders");


    // header
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
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::BeginChild("##coordinatesliders");

    if (coord_scratch.empty()) {
        ImGui::NewLine();
        ImGui::TextDisabled("    (no coordinates in this model)");
    }

    int i = 0;
    bool state_modified = false;
    ImGui::Columns(3);
    for (OpenSim::Coordinate const* c : coord_scratch) {
        ImGui::PushID(i++);

        int styles_pushed = 0;
        if (c == uim.getHovered()) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
            ++styles_pushed;
        }
        if (c == uim.getSelected()) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
            ++styles_pushed;
        }
        ImGui::Text("%s", c->getName().c_str());
        ImGui::PopStyleColor(styles_pushed);
        styles_pushed = 0;

        if (ImGui::IsItemHovered()) {
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
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            uim.setSelected(c);
        }

        ImGui::NextColumn();

        if (c->getLocked(uim.getState())) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
            ++styles_pushed;
        }

        if (ImGui::Button(c->getLocked(uim.getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK)) {
            uim.pushCoordinateEdit(*c, CoordinateEdit{c->getValue(uim.getState()), c->getSpeedValue(uim.getState()), !c->getLocked(uim.getState())});
            state_modified = true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);
            ImGui::Text("Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::SameLine();

        float v = static_cast<float>(c->getValue(uim.getState()));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("##coordinatevalueeditor", &v, static_cast<float>(c->getRangeMin()), static_cast<float>(c->getRangeMax()))) {
            uim.pushCoordinateEdit(*c, CoordinateEdit{static_cast<double>(v), c->getSpeedValue(uim.getState()), c->getLocked(uim.getState())});
            state_modified = true;
        }


        ImGui::PopStyleColor(styles_pushed);
        styles_pushed = 0;
        ImGui::NextColumn();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

        float speed = static_cast<float>(c->getSpeedValue(uim.getState()));
        if (ImGui::InputFloat("##coordinatespeededitor", &speed)) {
            uim.pushCoordinateEdit(*c, CoordinateEdit{c->getValue(uim.getState()), speed, c->getLocked(uim.getState())});
            state_modified = true;
        }
        ImGui::NextColumn();

        ImGui::PopID();
    }
    ImGui::Columns();

    ImGui::EndChild();

    return state_modified;
}
