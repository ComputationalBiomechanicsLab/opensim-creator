#include "CoordinateEditor.hpp"

#include "src/Styling.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <imgui.h>

static bool do_sort_by_name(OpenSim::Coordinate const* c1, OpenSim::Coordinate const* c2) {
    return c1->getName() < c2->getName();
}

static bool should_filter_out(osc::CoordinateEditor const& st, OpenSim::Coordinate const* c) {
    if (c->getName().find(st.filter) == c->getName().npos) {
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
    ImGui::TextUnformatted(ICON_FA_SEARCH);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    ImGui::InputText("##coords search filter", filter, sizeof(filter));
    ImGui::Dummy({0.0f, 3.0f});
    ImGui::Separator();
    ImGui::Dummy({0.0f, 3.0f});

    // load coords
    coord_scratch.clear();
    get_coordinates(*uim.model, coord_scratch);

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

    bool state_modified = false;
    ImGui::Columns(2);
    int i = 0;
    for (OpenSim::Coordinate const* c : coord_scratch) {
        ImGui::PushID(i++);

        int styles_pushed = 0;
        if (c == uim.hovered) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
            ++styles_pushed;
        }
        if (c == uim.selected) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
            ++styles_pushed;
        }
        ImGui::Text("%s", c->getName().c_str());
        ImGui::PopStyleColor(styles_pushed);
        styles_pushed = 0;

        if (ImGui::IsItemHovered()) {
            uim.hovered = const_cast<OpenSim::Coordinate*>(c);

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
            uim.selected = const_cast<OpenSim::Coordinate*>(c);
        }

        ImGui::NextColumn();

        // if locked, color everything red
        if (c->getLocked(*uim.state)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
            ++styles_pushed;
        }

        if (ImGui::Button(c->getLocked(*uim.state) ? ICON_FA_LOCK : ICON_FA_UNLOCK)) {
            uim.addCoordinateEdit(*c, CoordinateEdit{c->getValue(*uim.state), !c->getLocked(*uim.state)});
            state_modified = true;
        }

        ImGui::SameLine();

        float v = static_cast<float>(c->getValue(*uim.state));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat(" ", &v, static_cast<float>(c->getRangeMin()), static_cast<float>(c->getRangeMax()))) {
            uim.addCoordinateEdit(*c, CoordinateEdit{static_cast<double>(v), c->getLocked(*uim.state)});
            state_modified = true;
        }

        ImGui::PopStyleColor(styles_pushed);
        styles_pushed = 0;
        ImGui::NextColumn();

        ImGui::PopID();
    }
    ImGui::Columns();

    return state_modified;
}
