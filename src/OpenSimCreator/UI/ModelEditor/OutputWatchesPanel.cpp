#include "OutputWatchesPanel.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
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

    void UpdateCachedSimulationReportIfNecessary(const IModelStatePair& src, CachedSimulationReport& cache)
    {
        const UID modelVersion = src.getModelVersion();
        const UID stateVersion = src.getStateVersion();

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
        std::shared_ptr<const UndoableModelStatePair> model_,
        const ParentPtr<IMainUIStateAPI>& api_) :

        StandardPanelImpl{panelName_},
        m_API{api_},
        m_Model{std::move(model_)}
    {}

private:
    void impl_draw_content() final
    {
        UpdateCachedSimulationReportIfNecessary(*m_Model, m_CachedReport);

        if (m_API->getNumUserOutputExtractors() > 0 && ui::begin_table("##OutputWatchesTable", 2, ui::TableFlag::SizingStretchProp))
        {
            ui::table_setup_column("Output", ui::ColumnFlag::WidthStretch);
            ui::table_setup_column("Value");
            ui::table_headers_row();

            for (int outputIdx = 0; outputIdx < m_API->getNumUserOutputExtractors(); ++outputIdx)
            {
                int column = 0;
                OutputExtractor o = m_API->getUserOutputExtractor(outputIdx);

                ui::push_id(outputIdx);

                ui::table_next_row();

                ui::table_set_column_index(column++);
                if (ui::draw_small_button(OSC_ICON_TRASH))
                {
                    m_API->removeUserOutputExtractor(outputIdx);
                }
                ui::same_line();
                ui::draw_text_unformatted(o.getName());

                ui::table_set_column_index(column++);
                ui::draw_text_unformatted(o.getValueString(m_Model->getModel(), m_CachedReport.simulationReport));

                ui::pop_id();
            }

            ui::end_table();
        }
        else
        {
            ui::draw_text_disabled_and_panel_centered("No outputs being watched");
            ui::draw_text_disabled_and_centered("(Right-click a component and 'Watch Output')");
        }
    }

    ParentPtr<IMainUIStateAPI> m_API;
    std::shared_ptr<const UndoableModelStatePair> m_Model;
    CachedSimulationReport m_CachedReport;
};


osc::OutputWatchesPanel::OutputWatchesPanel(
    std::string_view panelName_,
    std::shared_ptr<const UndoableModelStatePair> model_,
    const ParentPtr<IMainUIStateAPI>& api_) :

    m_Impl{std::make_unique<Impl>(panelName_, std::move(model_), api_)}
{}
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
