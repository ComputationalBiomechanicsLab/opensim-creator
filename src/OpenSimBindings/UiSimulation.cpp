#include "UiSimulation.hpp"

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/UiModel.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <memory>
#include <vector>

using namespace osc;

static void applyCoordEdits(OpenSim::Model& m, SimTK::State& st, std::unordered_map<std::string, CoordinateEdit> const& coordEdits) {
    for (auto& p : coordEdits) {
        if (!m.hasComponent(p.first)) {
            continue;  // TODO: evict it
        }

        OpenSim::Coordinate const& c = m.getComponent<OpenSim::Coordinate>(p.first);

        if (c.getValue(st) != p.second.value) {
            c.setValue(st, p.second.value);
        }

        if (c.getLocked(st) != p.second.locked) {
            c.setLocked(st, p.second.locked);
        }
    }
}

static std::unique_ptr<SimTK::State> initializeState(OpenSim::Model& m, std::unordered_map<std::string, CoordinateEdit> const& coordEdits) {
    std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(m.initializeState());
    applyCoordEdits(m, *rv, coordEdits);
    m.equilibrateMuscles(*rv);
    m.realizeAcceleration(*rv);
    return rv;
}


static std::unique_ptr<FdSimulation> createForwardDynamicSim(UiModel const& uim, FdParams const& p) {
    auto modelCopy = std::make_unique<OpenSim::Model>(*uim.model);

    modelCopy->buildSystem();
    auto stateCopy = initializeState(*modelCopy, uim.coordEdits);

    auto simInput = std::make_unique<Input>(std::move(modelCopy), std::move(stateCopy));
    simInput->params = p;

    return std::make_unique<FdSimulation>(std::move(simInput));
}

static std::unique_ptr<OpenSim::Model> createInitializedModel(OpenSim::Model const& m) {
    auto rv = std::make_unique<OpenSim::Model>(m);
    rv->finalizeFromProperties();
    rv->finalizeConnections();
    rv->buildSystem();
    return rv;
}

static std::unique_ptr<Report> createDummySimulationReport(OpenSim::Model& m, std::unordered_map<std::string, CoordinateEdit> const& coordEdits) {
    auto rv = std::make_unique<Report>();

    rv->state = m.initializeState();
    applyCoordEdits(m, rv->state, coordEdits);
    m.equilibrateMuscles(rv->state);
    m.realizeReport(rv->state);

    rv->stats = {};
    m.realizeReport(rv->state);
    return rv;
}

// public API

osc::UiSimulation::UiSimulation(UiModel const& uim, FdParams const& p) :
    simulation{createForwardDynamicSim(uim, p)},
    model{createInitializedModel(*uim.model)},
    spotReport{createDummySimulationReport(*model, uim.coordEdits)},
    regularReports{},
    fixupScaleFactor{uim.fixupScaleFactor} {
}

osc::UiSimulation::UiSimulation(UiSimulation&&) noexcept = default;
osc::UiSimulation::~UiSimulation() noexcept = default;
osc::UiSimulation& osc::UiSimulation::operator=(UiSimulation&&) noexcept = default;
