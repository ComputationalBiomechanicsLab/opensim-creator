#include "fd_params_editor_popup.hpp"

#include "src/opensim_bindings/simulation.hpp"
#include "src/ui/help_marker.hpp"

#include <imgui.h>

bool osc::ui::fd_params_editor_popup::draw(char const* modal_name, fd::Params& p) {
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return false;
    }

    bool edited = false;

    // edit sim final time
    {
        float t = static_cast<float>(p.final_time.count());
        if (ImGui::InputFloat(p.final_time_title, &t, 0.0f, 0.0f, "%.9f")) {
            p.final_time = std::chrono::duration<double>{static_cast<double>(t)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.final_time_desc);
    }

    // edit integrator method
    {
        int method = p.integrator_method;
        if (ImGui::Combo(
                p.integrator_method_title,
                &method,
                fd::g_IntegratorMethodNames,
                fd::IntegratorMethod_NumIntegratorMethods)) {
            p.integrator_method = static_cast<fd::IntegratorMethod>(method);
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.integrator_method_desc);
    }

    // edit throttle
    {
        if (ImGui::Checkbox(p.throttle_to_wall_time_title, &p.throttle_to_wall_time)) {
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.throttle_to_wall_time_desc);
    }

    // edit reporting interval
    {
        float interval = static_cast<float>(p.reporting_interval.count());
        if (ImGui::InputFloat(p.reporting_interval_title, &interval)) {
            p.reporting_interval = std::chrono::duration<double>{static_cast<double>(interval)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.reporting_interval_desc);
    }

    // edit integrator step limit
    {
        if (ImGui::InputInt(p.integrator_step_limit_title, &p.integrator_step_limit)) {
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.integrator_step_limit_desc);
    }

    // edit integrator minimum step size
    {
        float min_step_size = static_cast<float>(p.integrator_minimum_step_size.count());
        if (ImGui::InputFloat(p.integrator_minimum_step_size_title, &min_step_size)) {
            p.integrator_minimum_step_size = std::chrono::duration<double>{static_cast<double>(min_step_size)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.integrator_minimum_step_size_desc);
    }

    // edit integrator max step size
    {
        float max_step_size = static_cast<float>(p.integrator_maximum_step_size.count());
        if (ImGui::InputFloat(p.integrator_maximum_step_size_title, &max_step_size)) {
            p.integrator_maximum_step_size = std::chrono::duration<double>{static_cast<double>(max_step_size)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.integrator_maximum_step_size_desc);
    }

    // edit integrator accuracy
    {
        float accuracy = static_cast<float>(p.integrator_accuracy);
        if (ImGui::InputFloat(p.integrator_accuracy_title, &accuracy)) {
            p.integrator_accuracy = static_cast<double>(accuracy);
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.integrator_accuracy_desc);
    }

    // edit updating on every step
    {
        if (ImGui::Checkbox(p.update_latest_state_on_every_step_title, &p.update_latest_state_on_every_step)) {
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.update_latest_state_on_every_step_desc);
    }

    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    if (ImGui::Button("save")) {
        ImGui::CloseCurrentPopup();
    }

    return edited;
}
