#include "MainEditorState.hpp"

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/OpenSimBindings/FdSimulation.hpp"
#include "src/OpenSimBindings/UiFdSimulation.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <optional>
#include <vector>
#include <utility>

class osc::MainEditorState::Impl final {
public:
    Impl() = default;

    Impl(std::unique_ptr<OpenSim::Model> model) :
        m_EditedModel{std::make_shared<UndoableUiModel>(std::move(model))}
    {
    }

    Impl(UndoableUiModel um) :
        m_EditedModel{std::make_shared<UndoableUiModel>(std::move(um))}
    {
    }

    UndoableUiModel const& getEditedModel() const
    {
        return *m_EditedModel;
    }

    UndoableUiModel& updEditedModel()
    {
        return *m_EditedModel;
    }

    std::shared_ptr<UndoableUiModel> updEditedModelPtr()
    {
        return m_EditedModel;
    }

    bool hasSimulations() const
    {
        return !m_Simulations.empty();
    }

    int getNumSimulations() const
    {
        return static_cast<int>(m_Simulations.size());
    }

    Simulation const& getSimulation(int idx) const
    {
        return m_Simulations.at(idx);
    }

    Simulation& updSimulation(int idx)
    {
        return m_Simulations.at(idx);
    }

    void addSimulation(Simulation s)
    {
        m_Simulations.push_back(std::move(s));
    }

    void removeSimulation(int idx)
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_Simulations.size()));
        m_Simulations.erase(m_Simulations.begin() + idx);
    }

    int getFocusedSimulationIndex() const
    {
        return m_FocusedSimulation;
    }

    Simulation const* getFocusedSimulation() const
    {
        return const_cast<Impl&>(*this).updFocusedSimulation();
    }

    Simulation* updFocusedSimulation()
    {
        if (!(0 <= m_FocusedSimulation && m_FocusedSimulation < static_cast<int>(m_Simulations.size())))
        {
            // out of bounds: default to something sane
            if (m_Simulations.empty())
            {
                return nullptr;
            }
            else
            {
                return &m_Simulations.back();
            }
        }
        else
        {
            // in bounds
            return &m_Simulations.at(m_FocusedSimulation);
        }
    }

    void setFocusedSimulation(int idx)
    {
        m_FocusedSimulation = std::move(idx);
    }

    ParamBlock const& getSimulationParams() const
    {
        return m_SimulationParams;
    }

    ParamBlock& updSimulationParams()
    {
        return m_SimulationParams;
    }

    int getNumUserOutputExtractors() const
    {
        return static_cast<int>(m_UserOutputExtractors.size());
    }

    OutputExtractor const& getUserOutputExtractor(int idx) const
    {
        return m_UserOutputExtractors.at(idx);
    }

    void addUserOutputExtractor(OutputExtractor output)
    {
        m_UserOutputExtractors.push_back(std::move(output));
    }

    void removeUserOutputExtractor(int idx)
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_UserOutputExtractors.size()));
        m_UserOutputExtractors.erase(m_UserOutputExtractors.begin() + idx);
    }

    UserPanelPreferences const& getUserPanelPrefs() const
    {
        return m_PanelPreferences;
    }

    UserPanelPreferences& updUserPanelPrefs()
    {
        return m_PanelPreferences;
    }

    int getNumViewers() const
    {
        return static_cast<int>(m_ModelViewers.size());
    }

    UiModelViewer& updViewer(int idx)
    {
        return m_ModelViewers.at(idx);
    }

    UiModelViewer& addViewer()
    {
        return m_ModelViewers.emplace_back();
    }

    void removeViewer(int idx)
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_ModelViewers.size()));
        m_ModelViewers.erase(m_ModelViewers.begin() + idx);
    }

private:
    std::shared_ptr<UndoableUiModel> m_EditedModel = std::make_shared<UndoableUiModel>();
    std::vector<Simulation> m_Simulations;
    int m_FocusedSimulation = -1;
    std::vector<OutputExtractor> m_UserOutputExtractors;
    ParamBlock m_SimulationParams = ToParamBlock(FdParams{});  // TODO: make generic
    std::vector<UiModelViewer> m_ModelViewers = []() { std::vector<UiModelViewer> rv; rv.emplace_back(); return rv; }();
    UserPanelPreferences m_PanelPreferences;
};

osc::MainEditorState::MainEditorState() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::MainEditorState::MainEditorState(std::unique_ptr<OpenSim::Model> model) :
    m_Impl{std::make_unique<Impl>(std::move(model))}
{
}

osc::MainEditorState::MainEditorState(UndoableUiModel um) :
    m_Impl{std::make_unique<Impl>(std::move(um))}
{
}

osc::MainEditorState::MainEditorState(MainEditorState&&) = default;
osc::MainEditorState& osc::MainEditorState::operator=(MainEditorState&&) = default;
osc::MainEditorState::~MainEditorState() noexcept = default;

osc::UndoableUiModel const& osc::MainEditorState::getEditedModel() const
{
    return m_Impl->getEditedModel();
}

osc::UndoableUiModel& osc::MainEditorState::updEditedModel()
{
    return m_Impl->updEditedModel();
}

std::shared_ptr<osc::UndoableUiModel> osc::MainEditorState::updEditedModelPtr()
{
    return m_Impl->updEditedModelPtr();
}

bool osc::MainEditorState::hasSimulations() const
{
    return m_Impl->hasSimulations();
}

int osc::MainEditorState::getNumSimulations() const
{
    return m_Impl->getNumSimulations();
}

osc::Simulation const& osc::MainEditorState::getSimulation(int idx) const
{
    return m_Impl->getSimulation(std::move(idx));
}

osc::Simulation& osc::MainEditorState::updSimulation(int idx)
{
    return m_Impl->updSimulation(std::move(idx));
}

void osc::MainEditorState::addSimulation(Simulation simulation)
{
    m_Impl->addSimulation(std::move(simulation));
}

void osc::MainEditorState::removeSimulation(int idx)
{
    m_Impl->removeSimulation(std::move(idx));
}

int osc::MainEditorState::getFocusedSimulationIndex() const
{
    return m_Impl->getFocusedSimulationIndex();
}

osc::Simulation const* osc::MainEditorState::getFocusedSimulation() const
{
    return m_Impl->getFocusedSimulation();
}

osc::Simulation* osc::MainEditorState::updFocusedSimulation()
{
    return m_Impl->updFocusedSimulation();
}

void osc::MainEditorState::setFocusedSimulation(int idx)
{
    m_Impl->setFocusedSimulation(std::move(idx));
}

osc::ParamBlock const& osc::MainEditorState::getSimulationParams() const
{
    return m_Impl->getSimulationParams();
}

osc::ParamBlock& osc::MainEditorState::updSimulationParams()
{
    return m_Impl->updSimulationParams();
}

int osc::MainEditorState::getNumUserOutputExtractors() const
{
    return m_Impl->getNumUserOutputExtractors();
}

osc::OutputExtractor const& osc::MainEditorState::getUserOutputExtractor(int idx) const
{
    return m_Impl->getUserOutputExtractor(std::move(idx));
}

void osc::MainEditorState::addUserOutputExtractor(OutputExtractor output)
{
    m_Impl->addUserOutputExtractor(std::move(output));
}

void osc::MainEditorState::removeUserOutputExtractor(int idx)
{
    m_Impl->removeUserOutputExtractor(std::move(idx));
}

osc::UserPanelPreferences const& osc::MainEditorState::getUserPanelPrefs() const
{
    return m_Impl->getUserPanelPrefs();
}

osc::UserPanelPreferences& osc::MainEditorState::updUserPanelPrefs()
{
    return m_Impl->updUserPanelPrefs();
}

int osc::MainEditorState::getNumViewers() const
{
    return m_Impl->getNumViewers();
}

osc::UiModelViewer& osc::MainEditorState::updViewer(int idx)
{
    return m_Impl->updViewer(std::move(idx));
}

osc::UiModelViewer& osc::MainEditorState::addViewer()
{
    return m_Impl->addViewer();
}

void osc::MainEditorState::removeViewer(int idx)
{
    m_Impl->removeViewer(std::move(idx));
}

void osc::AutoFocusAllViewers(MainEditorState& st)
{
    for (int i = 0, len = st.getNumViewers(); i < len; ++i)
    {
        st.updViewer(i).requestAutoFocus();
    }
}

void osc::StartSimulatingEditedModel(MainEditorState& st)
{
    UndoableUiModel const& uim = st.getEditedModel();
    BasicModelStatePair modelState{uim.getModel(), uim.getState()};
    FdParams params = FromParamBlock(st.getSimulationParams());

    st.addSimulation(UiFdSimulation{std::move(modelState), std::move(params)});
    st.setFocusedSimulation(st.getNumSimulations()-1);
}

std::vector<osc::OutputExtractor> osc::GetAllUserDesiredOutputs(MainEditorState const& st)
{
    int nOutputs = st.getNumUserOutputExtractors();

    std::vector<OutputExtractor> rv;
    rv.reserve(nOutputs);
    for (int i = 0; i < nOutputs; ++i)
    {
        rv.push_back(st.getUserOutputExtractor(i));
    }
    return rv;
}
