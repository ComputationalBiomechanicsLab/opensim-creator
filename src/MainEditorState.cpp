#include "MainEditorState.hpp"

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/Output.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/OpenSimBindings/Simulation.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <optional>

class osc::MainEditorState::Impl final {
public:
    Impl()
    {
    }

    Impl(std::unique_ptr<OpenSim::Model>)
    {
    }

    Impl(UndoableUiModel um)
    {
    }

    UndoableUiModel const& getEditedModel() const
    {
    }

    UndoableUiModel& updEditedModel()
    {
    }

    bool hasSimulations() const
    {
    }

    int getNumSimulations() const
    {
    }

    Simulation const& getSimulation(int) const
    {
    }

    Simulation& updSimulation(int)
    {
    }

    void addSimulation(Simulation)
    {
    }

    void removeSimulation(int)
    {
    }

    Simulation* updFocusedSimulation()
    {
    }

    Simulation const* getFocusedSim() const
    {
    }

    int getNumUserDesiredOutputs() const
    {
    }

    Output const& getUserDesiredOutput(int)
    {
    }

    void addUserDesiredOutput(Output)
    {
    }

    ParamBlock const& getSimulationParams() const
    {
    }

    ParamBlock& updSimulationParams()
    {
    }

    UserPanelPreferences const& getUserPanelPrefs() const
    {
    }

    UserPanelPreferences& updUserPanelPrefs()
    {
    }

    std::optional<float> getUserSimulationScrubbingTime() const
    {
    }

    void setUserSimulationScrubbingTime(float)
    {
    }

    void clearUserSimulationScrubbingTime()
    {
    }

    int getNumViewers() const
    {
    }

    UiModelViewer& updViewer(int)
    {
    }

    UiModelViewer& addViewer()
    {
    }

    void startSimulatingEditedModel()
    {
    }
private:
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

osc::Simulation* osc::MainEditorState::updFocusedSimulation()
{
    return m_Impl->updFocusedSimulation();
}

osc::Simulation const* osc::MainEditorState::getFocusedSim() const
{
    return m_Impl->getFocusedSim();
}

int osc::MainEditorState::getNumUserDesiredOutputs() const
{
    return m_Impl->getNumUserDesiredOutputs();
}

osc::Output const& osc::MainEditorState::getUserDesiredOutput(int idx)
{
    return m_Impl->getUserDesiredOutput(std::move(idx));
}

void osc::MainEditorState::addUserDesiredOutput(Output output)
{
    m_Impl->addUserDesiredOutput(std::move(output));
}

osc::ParamBlock const& osc::MainEditorState::getSimulationParams() const
{
    return m_Impl->getSimulationParams();
}

osc::ParamBlock& osc::MainEditorState::updSimulationParams()
{
    return m_Impl->updSimulationParams();
}

osc::UserPanelPreferences const& osc::MainEditorState::getUserPanelPrefs() const
{
    return m_Impl->getUserPanelPrefs();
}

osc::UserPanelPreferences& osc::MainEditorState::updUserPanelPrefs()
{
    return m_Impl->updUserPanelPrefs();
}

std::optional<float> osc::MainEditorState::getUserSimulationScrubbingTime() const
{
    return m_Impl->getUserSimulationScrubbingTime();
}

void osc::MainEditorState::setUserSimulationScrubbingTime(float t)
{
    m_Impl->setUserSimulationScrubbingTime(std::move(t));
}

void osc::MainEditorState::clearUserSimulationScrubbingTime()
{
    m_Impl->clearUserSimulationScrubbingTime();
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

void osc::MainEditorState::startSimulatingEditedModel()
{
    m_Impl->startSimulatingEditedModel();
}

/*
osc::MainEditorState::MainEditorState() :
    MainEditorState{UndoableUiModel{}}
{
}

osc::MainEditorState::MainEditorState(std::unique_ptr<OpenSim::Model> model) :
    MainEditorState{UndoableUiModel{std::move(model)}}
{
}

osc::MainEditorState::MainEditorState(UndoableUiModel uim) :
    editedModel{std::move(uim)},
    simulations{},
    focusedSimulation{-1},
    focusedSimulationScrubbingTime{-1.0f},
    desiredOutputs{},
    simParams{},
    viewers{std::make_unique<UiModelViewer>(), nullptr, nullptr, nullptr}
{
}

void osc::MainEditorState::setModel(std::unique_ptr<OpenSim::Model> newModel)
{
    editedModel.setModel(std::move(newModel));
}


// todo





// the model that the user is currently editing
UndoableUiModel editedModel;

// running/finished simulations
//
// the models being simulated are separate from the model being edited
std::vector<std::unique_ptr<UiSimulation>> simulations;

// currently-focused simulation
int focusedSimulation = -1;

// simulation time the user is scrubbed to - if they have scrubbed to
// a specific time
//
// if the scrubbing time doesn't fall within the currently-available
// simulation states ("frames") then the implementation will just use
// the latest available time
float focusedSimulationScrubbingTime = -1.0f;

std::shared_ptr<SynchronizedValue<std::vector<Output>>> userDesiredOutputs;

// parameters used when launching a new simulation
//
// these are the params that are used whenever a user hits "simulate"
FdParams simParams;

// available 3D viewers
//
// the user can open a limited number of 3D viewers. They are kept on
// this top-level state so that they, and their settings, can be
// cached between screens
//
// the viewers can be null, which should be interpreted as "not yet initialized"
std::array<std::unique_ptr<UiModelViewer>, 4> viewers;

// which panels should be shown
//
// the user can enable/disable these in the "window" entry in the main menu
struct {

} showing;


void startSimulatingEditedModel()
{
    int newFocus = static_cast<int>(simulations.size());
    simulations.emplace_back(new UiSimulation{editedModel.getUiModel(), simParams});
    focusedSimulation = newFocus;
    focusedSimulationScrubbingTime = -1.0f;
}


[[nodiscard]] UiSimulation* getFocusedSim()
{
    if (!(0 <= focusedSimulation && focusedSimulation < static_cast<int>(simulations.size())))
    {
        return nullptr;
    }
    else
    {
        return simulations[static_cast<size_t>(focusedSimulation)].get();
    }
}

[[nodiscard]] UiSimulation const* getFocusedSim() const
{
    return const_cast<MainEditorState*>(this)->getFocusedSim();
}

[[nodiscard]] bool hasSimulations() const
{
    return !simulations.empty();
}
*/
