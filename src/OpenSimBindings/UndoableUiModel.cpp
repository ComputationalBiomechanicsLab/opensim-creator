#include "UndoableUiModel.hpp"

#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Log.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>

using namespace osc;
using std::chrono_literals::operator""s;

namespace {
    struct DebounceRv final {
        std::chrono::system_clock::time_point validAt;
        bool shouldUndo;
    };
}

static DebounceRv canPushNewUndoStateWithDebounce(UndoableUiModel const& uim) {
    DebounceRv rv;
    rv.validAt = std::chrono::system_clock::now();
    rv.shouldUndo = uim.undo.empty() || uim.undo.back().getTimestamp() + 5s <= rv.validAt;
    return rv;
}

static void doDebouncedUndoPush(UndoableUiModel& uim) {
    auto [validAt, shouldUndo] = canPushNewUndoStateWithDebounce(uim);

    if (shouldUndo) {
        uim.undo.emplace_back(uim.current).setTimestamp(validAt);
        uim.redo.clear();
    }
}


static void rollbackModelToEarlierState(UndoableUiModel& uim) {
    if (uim.undo.empty()) {
        log::error("the model cannot be fixed: no earlier versions of the model exist, throwing an exception");
        throw std::runtime_error{
            "an OpenSim::Model was put into an invalid state: probably by a modification. We tried to recover from this error, but couldn't - view the logs"};
    }

    // otherwise, we have undo entries we can pop
    log::error(
        "attempting to rollback to an earlier (pre-modification) of the model that was saved into the undo buffer");
    try {
        uim.damaged.emplace(std::move(uim.current));
        uim.current = uim.undo.pop_back();
        return;
    } catch (...) {
        log::error(
            "error encountered when trying to rollback to an earlier version of the model, this will be thrown as an exception");
        throw;
    }
}

static void carefullyTryToInitSimTKSystemAndRealizeOnCurrentModel(UndoableUiModel& uim) {
    // this code is messy because it has to handle the messy situation where
    // the `current` model has been modified into an invalid state that OpenSim
    // refuses to initialize a system for
    //
    // this code has to balance being super-aggressive (i.e. immediately terminating with
    // a horrible error message) against letting the UI limp along with the broken
    // model *just* long enough for a recovery effort to complete. Typical end-users
    // are going to *strongly* prefer the latter, because they might have unsaved
    // changes in the UI that should not be lost by a crash.

    try {
        uim.current.updateIfDirty();
        return;
    } catch (std::exception const& ex) {
        log::error("exception thrown when initializing updated model: %s", ex.what());
    }

    rollbackModelToEarlierState(uim);
}

// public API

osc::UndoableUiModel::UndoableUiModel() :
    current{},
    undo{},
    redo{},
    damaged{std::nullopt} {
}

osc::UndoableUiModel::UndoableUiModel(std::unique_ptr<OpenSim::Model> model) :
    current{std::move(model)},
    undo{},
    redo{},
    damaged{std::nullopt} {
}

void osc::UndoableUiModel::doUndo() {
    if (!canUndo()) {
        return;
    }

    redo.emplace_back(std::move(current));
    current = undo.pop_back();
}

void osc::UndoableUiModel::doRedo() {
    if (!canRedo()) {
        return;
    }

    undo.emplace_back(std::move(current));
    current = redo.pop_back();
}

void osc::UndoableUiModel::setModel(std::unique_ptr<OpenSim::Model> newModel) {
    // care: this step can throw, because it initializes a system etc.
    //       so, do this *before* potentially breaking these sequecnes
    UiModel newCurrentModel{std::move(newModel)};

    undo.emplace_back(std::move(current));
    redo.clear();
    current = std::move(newCurrentModel);
}

void osc::UndoableUiModel::beforeModifyingModel() {
    osc::log::debug("starting model modification");
    doDebouncedUndoPush(*this);
}

void osc::UndoableUiModel::afterModifyingModel() {
    osc::log::debug("ended model modification");
    carefullyTryToInitSimTKSystemAndRealizeOnCurrentModel(*this);
}

void osc::UndoableUiModel::forciblyRollbackToEarlierState() {
    rollbackModelToEarlierState(*this);
}

void osc::UndoableUiModel::clearAnyDamagedModels() {
    if (damaged) {
        log::error("destructing damaged model");
        damaged = std::nullopt;
    }
}
