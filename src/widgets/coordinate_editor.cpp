#include "coordinate_editor.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <imgui.h>

static bool sort_by_name(OpenSim::Coordinate const* c1, OpenSim::Coordinate const* c2) {
    return c1->getName() < c2->getName();
}

static bool should_filter_out(osmv::Coordinate_editor_state& st, OpenSim::Coordinate const* c) {
    if (c->getName().find(st.filter) == c->getName().npos) {
        return true;
    }

    OpenSim::Coordinate::MotionType mt = c->getMotionType();
    if (st.show_rotational and mt == OpenSim::Coordinate::MotionType::Rotational) {
        return false;
    }

    if (st.show_translational and mt == OpenSim::Coordinate::MotionType::Translational) {
        return false;
    }

    if (st.show_coupled and mt == OpenSim::Coordinate::MotionType::Coupled) {
        return false;
    }

    return true;
}

void osmv::get_coordinates(OpenSim::Model const& m, std::vector<OpenSim::Coordinate const*>& out) {
    OpenSim::CoordinateSet const& s = m.getCoordinateSet();
    int len = s.getSize();
    out.reserve(out.size() + static_cast<size_t>(len));
    for (int i = 0; i < len; ++i) {
        out.push_back(&s[i]);
    }
}

bool osmv::draw_coordinate_editor(Coordinate_editor_state& st, OpenSim::Model const& model, SimTK::State& stk_st) {
    // render coordinate filters
    {
        ImGui::Text("filters:");
        ImGui::Dummy({0.0f, 2.5f});
        ImGui::Separator();

        ImGui::Columns(2);

        ImGui::Text("search");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##coords search filter", st.filter, sizeof(st.filter));
        ImGui::NextColumn();

        ImGui::Text("sort alphabetically");
        ImGui::NextColumn();
        ImGui::Checkbox("##coords alphabetical sort", &st.sort_by_name);
        ImGui::NextColumn();

        ImGui::Text("show rotational");
        ImGui::NextColumn();
        ImGui::Checkbox("##rotational coordinates checkbox", &st.show_rotational);
        ImGui::NextColumn();

        ImGui::Text("show translational");
        ImGui::NextColumn();
        ImGui::Checkbox("##translational coordinates checkbox", &st.show_translational);
        ImGui::NextColumn();

        ImGui::Text("show coupled");
        ImGui::NextColumn();
        ImGui::Checkbox("##coupled coordinates checkbox", &st.show_coupled);
        ImGui::NextColumn();

        ImGui::Columns();
    }

    // load coords
    st.coord_scratch.clear();
    get_coordinates(model, st.coord_scratch);

    // sort coords
    {
        auto pred = [&st](auto const* c) { return should_filter_out(st, c); };
        auto it = std::remove_if(st.coord_scratch.begin(), st.coord_scratch.end(), pred);
        st.coord_scratch.erase(it, st.coord_scratch.end());
    }

    // sort coords
    if (st.sort_by_name) {
        std::sort(st.coord_scratch.begin(), st.coord_scratch.end(), sort_by_name);
    }

    // render coordinates list
    ImGui::Dummy({0.0f, 10.0f});
    ImGui::Text("coordinates (%zu):", st.coord_scratch.size());
    ImGui::Dummy({0.0f, 2.5f});
    ImGui::Separator();

    bool state_modified = false;
    ImGui::Columns(2);
    int i = 0;
    for (OpenSim::Coordinate const* c : st.coord_scratch) {
        ImGui::PushID(i++);

        ImGui::Text("%s", c->getName().c_str());
        ImGui::NextColumn();

        // if locked, color everything red
        int styles_pushed = 0;
        if (c->getLocked(stk_st)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.6f, 0.0f, 0.0f, 1.0f});
            ++styles_pushed;
        }

        if (ImGui::Button(c->getLocked(stk_st) ? "u" : "l")) {
            c->setLocked(stk_st, not c->getLocked(stk_st));
            state_modified = true;
        }

        ImGui::SameLine();

        float v = static_cast<float>(c->getValue(stk_st));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat(" ", &v, static_cast<float>(c->getRangeMin()), static_cast<float>(c->getRangeMax()))) {
            c->setValue(stk_st, static_cast<double>(v));
            state_modified = true;
        }

        ImGui::PopStyleColor(styles_pushed);
        ImGui::NextColumn();

        ImGui::PopID();
    }
    ImGui::Columns();

    return state_modified;
}
