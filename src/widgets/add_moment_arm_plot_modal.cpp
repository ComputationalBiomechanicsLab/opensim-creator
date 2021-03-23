#include "add_moment_arm_plot_modal.hpp"

#include "src/widgets/coordinate_editor.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <imgui.h>

static bool sort_by_name(OpenSim::Component const* c1, OpenSim::Component const* c2) {
    return c1->getName() < c2->getName();
}

void osmv::draw_add_moment_arm_plot_modal(
    Add_moment_arm_plot_modal_state& st,
    char const* modal_name,
    OpenSim::Model const& model,
    std::function<void(std::pair<OpenSim::Muscle const*, OpenSim::Coordinate const*>)> const& on_add_plot_requested) {

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return;
    }

    ImGui::Columns(2);

    // lhs: muscle selection
    {
        ImGui::Text("muscles:");
        ImGui::Dummy({0.0f, 5.0f});

        auto& muscles = st.muscles_scratch;

        muscles.clear();
        for (OpenSim::Muscle const& musc : model.getComponentList<OpenSim::Muscle>()) {
            muscles.push_back(&musc);
        }

        // usability: sort by name
        std::sort(muscles.begin(), muscles.end(), sort_by_name);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild(
            "MomentArmPlotMuscleSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);

        for (OpenSim::Muscle const* m : muscles) {
            if (ImGui::Selectable(m->getName().c_str(), m == st.selected_muscle)) {
                st.selected_muscle = m;
            }
        }
        ImGui::EndChild();
    }
    ImGui::NextColumn();

    // rhs: coord selection
    {
        ImGui::Text("coordinates:");
        ImGui::Dummy({0.0f, 5.0f});

        auto& coords = st.coords_scratch;

        coords.clear();
        get_coordinates(model, coords);

        // usability: sort by name
        std::sort(coords.begin(), coords.end(), sort_by_name);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild(
            "MomentArmPlotCoordSelection", ImVec2(ImGui::GetContentRegionAvail().x, 260), false, window_flags);

        for (OpenSim::Coordinate const* c : coords) {
            if (ImGui::Selectable(c->getName().c_str(), c == st.selected_coord)) {
                st.selected_coord = c;
            }
        }

        ImGui::EndChild();
    }
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (ImGui::Button("cancel")) {
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    if (st.selected_coord && st.selected_muscle) {
        ImGui::SameLine();
        if (ImGui::Button("OK")) {
            on_add_plot_requested({st.selected_muscle, st.selected_coord});
            st = {};  // reset user input
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}
