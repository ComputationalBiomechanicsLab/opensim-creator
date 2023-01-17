#include "SimulationDetailsPanel.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/SimulationOutputPlot.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/Platform/os.hpp"
#include "src/Widgets/StandardPanel.hpp"
#include "src/Utils/Perf.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

class osc::SimulationDetailsPanel::Impl final : public StandardPanel {
public:
    Impl(
        std::string_view panelName,
        SimulatorUIAPI* simulatorUIAPI,
        std::shared_ptr<Simulation> simulation) :

        StandardPanel{std::move(panelName)},
        m_SimulatorUIAPI{simulatorUIAPI},
        m_Simulation{std::move(simulation)}
    {
    }

private:
    void implDrawContent() final
    {
        {
            ImGui::Dummy({0.0f, 1.0f});
            ImGui::TextUnformatted("info:");
            ImGui::SameLine();
            osc::DrawHelpMarker("Top-level info about the simulation");
            ImGui::Separator();
            ImGui::Dummy({0.0f, 2.0f});

            ImGui::Columns(2);
            ImGui::Text("num reports");
            ImGui::NextColumn();
            ImGui::Text("%i", m_Simulation->getNumReports());
            ImGui::NextColumn();
            ImGui::Columns();
        }

        {
            OSC_PERF("draw simulation params");
            DrawSimulationParams(m_Simulation->getParams());
        }

        ImGui::Dummy({0.0f, 10.0f});

        {
            OSC_PERF("draw simulation stats");
            drawSimulationStatPlots(*m_Simulation);
        }
    }

    void drawSimulationStatPlots(osc::Simulation& sim)
    {
        auto outputs = sim.getOutputs();

        if (outputs.empty())
        {
            ImGui::TextDisabled("(no simulator output plots available for this simulation)");
            return;
        }

        ImGui::Dummy({0.0f, 1.0f});
        ImGui::Columns(2);
        ImGui::TextUnformatted("plots:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Various statistics collected when the simulation was ran");
        ImGui::NextColumn();
        if (std::any_of(outputs.begin(), outputs.end(), [](osc::OutputExtractor const& o) { return o.getOutputType() == osc::OutputType::Float; }))
        {
            ImGui::Button(ICON_FA_SAVE " Save All " ICON_FA_CARET_DOWN);
            if (ImGui::BeginPopupContextItem("##exportoptions", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("as CSV"))
                {
                    osc::TryPromptAndSaveOutputsAsCSV(*m_SimulatorUIAPI, outputs);
                }

                if (ImGui::MenuItem("as CSV (and open)"))
                {
                    std::filesystem::path p = osc::TryPromptAndSaveOutputsAsCSV(*m_SimulatorUIAPI, outputs);
                    if (!p.empty())
                    {
                        osc::OpenPathInOSDefaultApplication(p);
                    }
                }

                ImGui::EndPopup();
            }
        }

        ImGui::NextColumn();
        ImGui::Columns();
        ImGui::Separator();
        ImGui::Dummy({0.0f, 2.0f});

        int imguiID = 0;
        ImGui::Columns(2);
        for (osc::OutputExtractor const& output : sim.getOutputs())
        {
            ImGui::PushID(imguiID++);
            DrawOutputNameColumn(output, false);
            ImGui::NextColumn();
            SimulationOutputPlot plot{m_SimulatorUIAPI, output, 32.0f};
            plot.draw();
            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::Columns();
    }

    SimulatorUIAPI* m_SimulatorUIAPI;
    std::shared_ptr<Simulation> m_Simulation;
};


// public API (PIMPL)

osc::SimulationDetailsPanel::SimulationDetailsPanel(
    std::string_view panelName,
    SimulatorUIAPI* simulatorUIAPI,
    std::shared_ptr<Simulation> simulation) :

    m_Impl{std::make_unique<Impl>(std::move(panelName), simulatorUIAPI, std::move(simulation))}
{
}

osc::SimulationDetailsPanel::SimulationDetailsPanel(SimulationDetailsPanel&&) noexcept = default;
osc::SimulationDetailsPanel& osc::SimulationDetailsPanel::operator=(SimulationDetailsPanel&&) noexcept = default;
osc::SimulationDetailsPanel::~SimulationDetailsPanel() noexcept = default;

void osc::SimulationDetailsPanel::draw()
{
    m_Impl->draw();
}