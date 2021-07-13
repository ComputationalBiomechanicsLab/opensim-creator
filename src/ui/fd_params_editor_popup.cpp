#include "fd_params_editor_popup.hpp"

#include "src/opensim_bindings/simulation.hpp"

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
        if (ImGui::InputFloat("final time", &t)) {
            p.final_time = std::chrono::duration<double>{static_cast<double>(t)};
            edited = true;
        }
    }

    // edit integrator method
    {
        int method = p.integrator_method;
        if (ImGui::Combo(
                "integration method",
                &method,
                fd::g_IntegratorMethodNames,
                fd::IntegratorMethod_NumIntegratorMethods)) {
            p.integrator_method = static_cast<fd::IntegratorMethod>(method);
            edited = true;
        }
    }

    // edit throttle
    {
        if (ImGui::Checkbox("throttle to wall time", &p.throttle_to_wall_time)) {
            edited = true;
        }
    }

    // edit reporting interval
    {
        float interval = static_cast<float>(p.reporting_interval.count());
        if (ImGui::InputFloat("reporting interval", &interval)) {
            p.reporting_interval = std::chrono::duration<double>{static_cast<double>(interval)};
            edited = true;
        }
    }

    // edit integrator step limit
    {
        if (ImGui::InputInt("integrator step limit", &p.integrator_step_limit)) {
            edited = true;
        }
    }

    // edit integrator minimum step size
    {
        float min_step_size = static_cast<float>(p.integrator_minimum_step_size.count());
        if (ImGui::InputFloat("integrator minimum step size", &min_step_size)) {
            p.integrator_minimum_step_size = std::chrono::duration<double>{static_cast<double>(min_step_size)};
            edited = true;
        }
    }

    // edit integrator max step size
    {
        float max_step_size = static_cast<float>(p.integrator_maximum_step_size.count());
        if (ImGui::InputFloat("integrator max step size", &max_step_size)) {
            p.integrator_maximum_step_size = std::chrono::duration<double>{static_cast<double>(max_step_size)};
            edited = true;
        }
    }

    // edit integrator accuracy
    {
        float accuracy = static_cast<float>(p.integrator_accuracy);
        if (ImGui::InputFloat("integrator accuracy", &accuracy)) {
            p.integrator_accuracy = static_cast<double>(accuracy);
            edited = true;
        }
    }

    // edit updating on every step
    {
        if (ImGui::Checkbox("update latest state on every integration step", &p.update_latest_state_on_every_step)) {
            edited = true;
        }
    }

    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    if (ImGui::Button("ok")) {
        ImGui::CloseCurrentPopup();
    }

    return edited;
}
