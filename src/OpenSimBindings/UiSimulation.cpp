#include "UiSimulation.hpp"

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/OpenSimBindings/UiModel.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <memory>
#include <vector>

using namespace osc;

static std::unique_ptr<SimTK::State> initializeState(OpenSim::Model& m,
                                                     StateModifications stateModifications)
{
    std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(m.initializeState());
    stateModifications.applyToState(m, *rv);
    m.equilibrateMuscles(*rv);
    m.realizeAcceleration(*rv);
    return rv;
}

static std::unique_ptr<FdSimulation> createForwardDynamicSim(UiModel const& uim,
                                                             FdParams const& p)
{
    auto modelCopy = std::make_unique<OpenSim::Model>(uim.getModel());
    modelCopy->finalizeFromProperties();
    modelCopy->finalizeConnections();
    modelCopy->buildSystem();
    auto stateCopy = initializeState(*modelCopy, uim.getStateModifications());

    auto simInput = std::make_unique<Input>(std::move(modelCopy), std::move(stateCopy));
    simInput->params = p;

    return std::make_unique<FdSimulation>(std::move(simInput));
}

static std::unique_ptr<OpenSim::Model> createInitializedModel(OpenSim::Model const& m)
{
    auto rv = std::make_unique<OpenSim::Model>(m);
    rv->finalizeFromProperties();
    rv->finalizeConnections();
    rv->buildSystem();
    return rv;
}

static std::unique_ptr<Report> createDummySimulationReport(OpenSim::Model& m,
                                                           StateModifications stateModifications)
{
    auto rv = std::make_unique<Report>();

    rv->state = m.initializeState();
    stateModifications.applyToState(m, rv->state);
    m.equilibrateMuscles(rv->state);
    m.realizeReport(rv->state);

    rv->stats = {};
    m.realizeReport(rv->state);
    return rv;
}

// public API

osc::UiSimulation::UiSimulation(UiModel const& uim, FdParams const& p) :
    simulation{createForwardDynamicSim(uim, p)},
    model{createInitializedModel(uim.getModel())},
    spotReport{createDummySimulationReport(*model, uim.getStateModifications())},
    regularReports{},
    fixupScaleFactor{uim.getFixupScaleFactor()}
{
}

osc::UiSimulation::UiSimulation(UiSimulation&&) noexcept = default;
osc::UiSimulation::~UiSimulation() noexcept = default;
osc::UiSimulation& osc::UiSimulation::operator=(UiSimulation&&) noexcept = default;
