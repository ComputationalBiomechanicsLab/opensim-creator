#include "UiSimulation.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Assertions.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <memory>
#include <vector>

using namespace osc;

static std::unique_ptr<Report> CreateDummySimulationReport(OpenSim::Model& m,
                                                           SimTK::State const& state)
{
    auto rv = std::make_unique<Report>();
    rv->state = state;
    rv->stats = {};
    m.realizeReport(rv->state);
    return rv;
}

// public API

osc::UiSimulation::UiSimulation(UiModel const& uim, FdParams const& fdParams) :
    simulation{std::make_unique<FdSimulation>(BasicModelStatePair{uim.getModel(), uim.getState()}, fdParams)},
    model{CreateInitializedModelCopy(uim.getModel())},
    spotReport{CreateDummySimulationReport(*model, uim.getState())},
    regularReports{},
    fixupScaleFactor{uim.getFixupScaleFactor()}
{
}

osc::UiSimulation::UiSimulation(UiSimulation&&) noexcept = default;
osc::UiSimulation::~UiSimulation() noexcept = default;
osc::UiSimulation& osc::UiSimulation::operator=(UiSimulation&&) noexcept = default;
