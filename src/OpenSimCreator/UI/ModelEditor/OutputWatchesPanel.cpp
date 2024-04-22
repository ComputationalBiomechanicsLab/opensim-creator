#include "OutputWatchesPanel.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/UID.h>
#include <Simbody.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    struct CachedSimulationReport final {
        UID sourceModelVersion;
        UID sourceStateVersion;
        SimulationReport simulationReport;
    };

    void UpdateCachedSimulationReportIfNecessary(IConstModelStatePair const& src, CachedSimulationReport& cache)
    {
        UID const modelVersion = src.getModelVersion();
        UID const stateVersion = src.getStateVersion();

        if (cache.sourceModelVersion == modelVersion &&
            cache.sourceStateVersion == stateVersion)
        {
            return;  // it's already up-to-date
        }

        SimTK::State s = src.getState();
        src.getModel().realizeReport(s);

        cache.simulationReport = SimulationReport{std::move(s)};
        cache.sourceModelVersion = modelVersion;
        cache.sourceStateVersion = stateVersion;
    }
}

class osc::OutputWatchesPanel::Impl final : public StandardPanelImpl {
public:

    Impl(std::string_view panelName_,
        std::shared_ptr<UndoableModelStatePair const> model_,
        ParentPtr<IMainUIStateAPI> const& api_) :

        StandardPanelImpl{panelName_},
        m_API{api_},
        m_Model{std::move(model_)}
    {
    }

private:
    void impl_draw_content() final
    {
        UpdateCachedSimulationReportIfNecessary(*m_Model, m_CachedReport);

        if (m_API->getNumUserOutputExtractors() > 0 && ui::BeginTable("##OutputWatchesTable", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthStretch);
            ui::TableSetupColumn("Value");
            ui::TableHeadersRow();

            for (int outputIdx = 0; outputIdx < m_API->getNumUserOutputExtractors(); ++outputIdx)
            {
                int column = 0;
                OutputExtractor o = m_API->getUserOutputExtractor(outputIdx);

                ui::PushID(outputIdx);

                ui::TableNextRow();

                ui::TableSetColumnIndex(column++);
                if (ui::SmallButton(ICON_FA_TRASH))
                {
                    m_API->removeUserOutputExtractor(outputIdx);
                }
                ui::SameLine();
                ui::TextUnformatted(o.getName());

                ui::TableSetColumnIndex(column++);
                ui::TextUnformatted(o.getValueString(m_Model->getModel(), m_CachedReport.simulationReport));

                ui::PopID();
            }

            ui::EndTable();
        }
        else
        {
            ui::TextDisabledAndWindowCentered("No outputs being watched");
            ui::TextDisabledAndCentered("(Right-click a component and 'Watch Output')");
        }
    }

    ParentPtr<IMainUIStateAPI> m_API;
    std::shared_ptr<UndoableModelStatePair const> m_Model;
    CachedSimulationReport m_CachedReport;
};


// public API (PIMPL)

osc::OutputWatchesPanel::OutputWatchesPanel(
    std::string_view panelName_,
    std::shared_ptr<UndoableModelStatePair const> model_,
    ParentPtr<IMainUIStateAPI> const& api_) :

    m_Impl{std::make_unique<Impl>(panelName_, std::move(model_), api_)}
{
}

osc::OutputWatchesPanel::OutputWatchesPanel(OutputWatchesPanel&&) noexcept = default;
osc::OutputWatchesPanel& osc::OutputWatchesPanel::operator=(OutputWatchesPanel&&) noexcept = default;
osc::OutputWatchesPanel::~OutputWatchesPanel() noexcept = default;

CStringView osc::OutputWatchesPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::OutputWatchesPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::OutputWatchesPanel::impl_open()
{
    m_Impl->open();
}

void osc::OutputWatchesPanel::impl_close()
{
    m_Impl->close();
}

void osc::OutputWatchesPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
