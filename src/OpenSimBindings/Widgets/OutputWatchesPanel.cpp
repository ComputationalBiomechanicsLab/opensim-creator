#include "OutputWatchesPanel.hpp"

#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/StandardPanel.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <Simbody.h>

#include <memory>
#include <string_view>
#include <utility>

namespace
{
    struct CachedSimulationReport final {
        osc::UID sourceModelVersion;
        osc::UID sourceStateVersion;
        osc::SimulationReport simulationReport;
    };

    void UpdateCachedSimulationReportIfNecessary(osc::VirtualConstModelStatePair const& src, CachedSimulationReport& cache)
    {
        osc::UID const modelVersion = src.getModelVersion();
        osc::UID const stateVersion = src.getStateVersion();

        if (cache.sourceModelVersion == modelVersion &&
            cache.sourceStateVersion == stateVersion)
        {
            return;  // it's already up-to-date
        }

        SimTK::State s = src.getState();
        src.getModel().realizeReport(s);

        cache.simulationReport = osc::SimulationReport{std::move(s)};
        cache.sourceModelVersion = modelVersion;
        cache.sourceStateVersion = stateVersion;
    }
}

class osc::OutputWatchesPanel::Impl final : public osc::StandardPanel {
public:

    Impl(std::string_view panelName_,
        std::shared_ptr<UndoableModelStatePair> model_,
        MainUIStateAPI* api_) :

        StandardPanel{std::move(panelName_)},
        m_API{std::move(api_)},
        m_Model{std::move(model_)}
    {
    }

private:
    void implDrawContent() override
    {
        UpdateCachedSimulationReportIfNecessary(*m_Model, m_CachedReport);

        if (m_API->getNumUserOutputExtractors() > 0 && ImGui::BeginTable("##OutputWatchesTable", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (int outputIdx = 0; outputIdx < m_API->getNumUserOutputExtractors(); ++outputIdx)
            {
                int column = 0;
                OutputExtractor o = m_API->getUserOutputExtractor(outputIdx);

                ImGui::PushID(outputIdx);

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(column++);
                if (ImGui::SmallButton(ICON_FA_TRASH))
                {
                    m_API->removeUserOutputExtractor(outputIdx);
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(o.getName().c_str());

                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(o.getValueString(m_Model->getModel(), m_CachedReport.simulationReport).c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        else
        {
            ImGui::TextWrapped("No outputs are being watched. You can watch outputs by right-clicking something in the model.");
        }
    }

    MainUIStateAPI* m_API;
    std::shared_ptr<UndoableModelStatePair> m_Model;
    CachedSimulationReport m_CachedReport;
};


// public API (PIMPL)

osc::OutputWatchesPanel::OutputWatchesPanel(
    std::string_view panelName,
    std::shared_ptr<UndoableModelStatePair> model,
    MainUIStateAPI* api) :

    m_Impl{std::make_unique<Impl>(std::move(panelName), std::move(model), std::move(api))}
{
}

osc::OutputWatchesPanel::OutputWatchesPanel(OutputWatchesPanel&&) noexcept = default;
osc::OutputWatchesPanel& osc::OutputWatchesPanel::operator=(OutputWatchesPanel&&) noexcept = default;
osc::OutputWatchesPanel::~OutputWatchesPanel() noexcept = default;

osc::CStringView osc::OutputWatchesPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::OutputWatchesPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::OutputWatchesPanel::implOpen()
{
    m_Impl->open();
}

void osc::OutputWatchesPanel::implClose()
{
    m_Impl->close();
}

void osc::OutputWatchesPanel::implDraw()
{
    m_Impl->draw();
}
