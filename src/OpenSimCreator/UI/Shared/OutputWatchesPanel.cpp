#include "OutputWatchesPanel.h"

#include <OpenSimCreator/Documents/Model/Environment.h>
#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/Panels/PanelPrivate.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/LifetimedPtr.h>
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

        if (cache.sourceModelVersion == modelVersion and
            cache.sourceStateVersion == stateVersion) {
            return;  // it's already up-to-date
        }

        SimTK::State s = src.getState();
        src.getModel().realizeReport(s);

        cache.simulationReport = SimulationReport{std::move(s)};
        cache.sourceModelVersion = modelVersion;
        cache.sourceStateVersion = stateVersion;
    }
}

class osc::OutputWatchesPanel::Impl final : public PanelPrivate {
public:

    explicit Impl(
        OutputWatchesPanel& owner,
        std::string_view panelName_,
        std::shared_ptr<const IModelStatePair> model_) :

        PanelPrivate{owner, nullptr, panelName_},
        m_Model{std::move(model_)}
    {}

    void draw_content()
    {
        UpdateCachedSimulationReportIfNecessary(*m_Model, m_CachedReport);

        std::shared_ptr<Environment> env = m_Model->tryUpdEnvironment();
        if (env->getNumUserOutputExtractors() > 0 and ui::begin_table("##OutputWatchesTable", 2, ui::TableFlag::SizingStretchProp)) {
            ui::table_setup_column("Output", ui::ColumnFlag::WidthStretch);
            ui::table_setup_column("Value");
            ui::table_headers_row();

            for (int outputIdx = 0; outputIdx < env->getNumUserOutputExtractors(); ++outputIdx) {
                int column = 0;
                OutputExtractor o = env->getUserOutputExtractor(outputIdx);

                ui::push_id(outputIdx);

                ui::table_next_row();

                ui::table_set_column_index(column++);
                if (ui::draw_small_button(OSC_ICON_TRASH)) {
                    env->removeUserOutputExtractor(outputIdx);
                }
                ui::same_line();
                ui::draw_text_unformatted(o.getName());

                ui::table_set_column_index(column++);
                ui::draw_text_unformatted(o.getValueString(m_Model->getModel(), m_CachedReport.simulationReport));

                ui::pop_id();
            }

            ui::end_table();
        }
        else {
            ui::draw_text_disabled_and_panel_centered("No outputs being watched");
            ui::draw_text_disabled_and_centered("(Right-click a component and 'Watch Output')");
        }
    }

private:
    std::shared_ptr<const IModelStatePair> m_Model;
    CachedSimulationReport m_CachedReport;
};

osc::OutputWatchesPanel::OutputWatchesPanel(
    std::string_view panelName_,
    std::shared_ptr<const IModelStatePair> model_) :

    Panel{std::make_unique<Impl>(*this, panelName_, std::move(model_))}
{}
void osc::OutputWatchesPanel::impl_draw_content() { private_data().draw_content(); }
