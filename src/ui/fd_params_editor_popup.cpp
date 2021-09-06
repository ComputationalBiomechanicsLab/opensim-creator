#include "fd_params_editor_popup.hpp"

#include "src/opensim_bindings/Simulation.hpp"
#include "src/ui/help_marker.hpp"

#include <imgui.h>

bool osc::ui::fd_params_editor_popup::draw(char const* modal_name, FdParams& p) {
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
        float t = static_cast<float>(p.FinalTime.count());
        if (ImGui::InputFloat(p.g_FinalTimeTitle, &t, 0.0f, 0.0f, "%.9f")) {
            p.FinalTime = std::chrono::duration<double>{static_cast<double>(t)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_FinalTimeDesc);
    }

    // edit integrator method
    {
        int method = p.IntegratorMethodUsed;
        if (ImGui::Combo(
                p.g_IntegratorMethodUsedTitle,
                &method,
                g_IntegratorMethodNames,
                IntegratorMethod_NumIntegratorMethods)) {
            p.IntegratorMethodUsed = static_cast<IntegratorMethod>(method);
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_IntegratorMethodUsedDesc);
    }

    // edit throttle
    {
        if (ImGui::Checkbox(p.g_ThrottleToWallTimeTitle, &p.ThrottleToWallTime)) {
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_ThrottleToWallTimeDesc);
    }

    // edit reporting interval
    {
        float interval = static_cast<float>(p.ReportingInterval.count());
        if (ImGui::InputFloat(p.g_ReportingIntervalTitle, &interval)) {
            p.ReportingInterval = std::chrono::duration<double>{static_cast<double>(interval)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_ReportingIntervalDesc);
    }

    // edit integrator step limit
    {
        if (ImGui::InputInt(p.g_IntegratorStepLimitTitle, &p.IntegratorStepLimit)) {
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_IntegratorStepLimitDesc);
    }

    // edit integrator minimum step size
    {
        float min_step_size = static_cast<float>(p.IntegratorMinimumStepSize.count());
        if (ImGui::InputFloat(p.g_IntegratorMinimumStepSizeTitle, &min_step_size)) {
            p.IntegratorMinimumStepSize = std::chrono::duration<double>{static_cast<double>(min_step_size)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_IntegratorMinimumStepSizeDesc);
    }

    // edit integrator max step size
    {
        float max_step_size = static_cast<float>(p.IntegratorMaximumStepSize.count());
        if (ImGui::InputFloat(p.g_IntegratorMaximumStepSizeTitle, &max_step_size)) {
            p.IntegratorMaximumStepSize = std::chrono::duration<double>{static_cast<double>(max_step_size)};
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_IntegratorMaximumStepSizeDesc);
    }

    // edit integrator accuracy
    {
        float accuracy = static_cast<float>(p.IntegratorAccuracy);
        if (ImGui::InputFloat(p.g_IntegratorAccuracyTitle, &accuracy)) {
            p.IntegratorAccuracy = static_cast<double>(accuracy);
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_IntegratorAccuracyDesc);
    }

    // edit updating on every step
    {
        if (ImGui::Checkbox(p.g_UpdateLatestStateOnEveryStepTitle, &p.UpdateLatestStateOnEveryStep)) {
            edited = true;
        }
        ImGui::SameLine();
        ui::help_marker::draw(p.g_UpdateLatestStateOnEveryStepDesc);
    }

    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    if (ImGui::Button("save")) {
        ImGui::CloseCurrentPopup();
    }

    return edited;
}
