#include "OutputWatchesPanel.hpp"

#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/NamedPanel.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <Simbody.h>

#include <memory>
#include <string_view>
#include <utility>

class osc::OutputWatchesPanel::Impl final : public osc::NamedPanel {
public:
    Impl(std::string_view panelName_,
        std::shared_ptr<UndoableModelStatePair> model_,
        MainUIStateAPI* api_) :

        NamedPanel{std::move(panelName_)},
        m_Model{std::move(model_)},
        m_API{std::move(api_)}
    {
    }

private:
    void implDraw() override
    {
        // this implementation is extremely slow because the outputs API requires a complete
        // simulation report (the editor has to manufacture it)
        SimTK::State s = m_Model->getState();
        m_Model->getModel().realizeReport(s);
        SimulationReport sr{std::move(s), {}};

        int nOutputs = m_API->getNumUserOutputExtractors();
        if (nOutputs > 0 && ImGui::BeginTable("output watches table", 3, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();

            for (int outputIdx = 0; outputIdx < nOutputs; ++outputIdx)
            {
                int column = 0;
                OutputExtractor const& o = m_API->getUserOutputExtractor(outputIdx);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(o.getName().c_str());
                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(o.getValueString(m_Model->getModel(), sr).c_str());
            }

            ImGui::EndTable();
        }


    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
    MainUIStateAPI* m_API;
};


// public API (PIMPL)

osc::OutputWatchesPanel::OutputWatchesPanel(
    std::string_view panelName,
    std::shared_ptr<UndoableModelStatePair> model,
    MainUIStateAPI* api) :

    m_Impl{new Impl{std::move(panelName), std::move(model), std::move(api)}}
{
}

osc::OutputWatchesPanel::OutputWatchesPanel(OutputWatchesPanel&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::OutputWatchesPanel& osc::OutputWatchesPanel::operator=(OutputWatchesPanel&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::OutputWatchesPanel::~OutputWatchesPanel() noexcept
{
    delete m_Impl;
}

bool osc::OutputWatchesPanel::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::OutputWatchesPanel::open()
{
    m_Impl->open();
}

void osc::OutputWatchesPanel::close()
{
    m_Impl->close();
}

bool osc::OutputWatchesPanel::draw()
{
    m_Impl->draw();
    return m_Impl->isOpen();
}
