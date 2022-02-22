#pragma once

#include <memory>
#include <vector>

namespace OpenSim
{
    class Model;
    class Component;
}

namespace osc
{
    class FdSimulation;
    struct Report;
    struct FdParams;
    class UiModel;
}

namespace osc
{
    // a forward-dynamic simulation
    //
    // the simulation's computation runs on a background thread, but
    // this struct also contains information that is kept UI-side for
    // UI feedback/interaction
    struct UiSimulation final {

        // the simulation, running on a background thread
        std::unique_ptr<FdSimulation> simulation;

        // copy of the model being simulated in the bg thread
        std::unique_ptr<OpenSim::Model> model;

        // current user selection, if any
        OpenSim::Component* selected = nullptr;

        // current user hover, if any
        OpenSim::Component* hovered = nullptr;

        // latest (usually per-integration-step) report popped from
        // the bg thread
        std::unique_ptr<Report> spotReport;

        // regular reports that are popped from the simulator thread by
        // the (polling) UI thread
        std::vector<std::unique_ptr<Report>> regularReports;

        // fixup scale factor of the model
        //
        // this scales up/down the decorations of the model - used for extremely
        // undersized models (e.g. fly leg)
        float fixupScaleFactor;

        // HACK: a pointer to the last report that the model was realized against
        //
        // This shouldn't be necessary--the model shouldn't "remember" anything about
        // what state it was realized against--but it is necessary beause there's a bug
        // in OpenSim that causes a state mutation (during realizeReport) to also mutate
        // the model slightly
        //
        // see: https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/123
        mutable Report* HACK_lastReportModelWasRealizedAgainst = nullptr;

        // start a new simulation by *copying* the provided Ui_model
        UiSimulation(UiModel const&, FdParams const&);
        UiSimulation(UiSimulation const&) = delete;
        UiSimulation(UiSimulation&&) noexcept;
        ~UiSimulation() noexcept;

        UiSimulation& operator=(UiSimulation&&) noexcept;
        UiSimulation& operator=(UiSimulation const&) = delete;
    };
}
