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

// public API

void osc::UndoableUiModel::rollbackModelToEarlierState() {
    if (m_UndoBuffer.empty()) {
        log::error("the model cannot be fixed: no earlier versions of the model exist, throwing an exception");
        throw std::runtime_error{
            "an OpenSim::Model was put into an invalid state: probably by a modification. We tried to recover from this error, but couldn't - view the logs"};
    }

    // otherwise, we have undo entries we can pop
    log::error(
        "attempting to rollback to an earlier (pre-modification) of the model that was saved into the undo buffer");
    try {
        m_Damaged.emplace(std::move(m_Current));
        m_Current = m_UndoBuffer.pop_back();
        return;
    } catch (...) {
        log::error(
            "error encountered when trying to rollback to an earlier version of the model, this will be thrown as an exception");
        throw;
    }
}

osc::UndoableUiModel::UndoableUiModel() :
    m_Current{},
    m_UndoBuffer{},
    m_RedoBuffer{},
    m_Damaged{std::nullopt} {
}

osc::UndoableUiModel::UndoableUiModel(std::unique_ptr<OpenSim::Model> model) :
    m_Current{std::move(model)},
    m_UndoBuffer{},
    m_RedoBuffer{},
    m_Damaged{std::nullopt} {
}

void osc::UndoableUiModel::doUndo() {
    if (!canUndo()) {
        return;
    }

    m_RedoBuffer.emplace_back(std::move(m_Current));
    m_Current = m_UndoBuffer.pop_back();
}

void osc::UndoableUiModel::doRedo() {
    if (!canRedo()) {
        return;
    }

    m_UndoBuffer.emplace_back(std::move(m_Current));
    m_Current = m_RedoBuffer.pop_back();
}

void osc::UndoableUiModel::setModel(std::unique_ptr<OpenSim::Model> newModel) {
    // care: this step can throw, because it initializes a system etc.
    //       so, do this *before* potentially breaking these sequecnes
    UiModel newCurrentModel{std::move(newModel)};

    m_UndoBuffer.emplace_back(std::move(m_Current));
    m_RedoBuffer.clear();
    m_Current = std::move(newCurrentModel);
}

void osc::UndoableUiModel::beforeModifyingModel() {
    osc::log::debug("starting model modification");
    DebounceRv rv;
    rv.validAt = std::chrono::system_clock::now();
    rv.shouldUndo = m_UndoBuffer.empty() || m_UndoBuffer.back().getTimestamp() + 5s <= rv.validAt;

    if (rv.shouldUndo) {
        m_UndoBuffer.emplace_back(m_Current).setTimestamp(rv.validAt);
        m_RedoBuffer.clear();
    }
}

void osc::UndoableUiModel::afterModifyingModel() {
    osc::log::debug("ended model modification");
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
        m_Current.updateIfDirty();
        return;
    } catch (std::exception const& ex) {
        log::error("exception thrown when initializing updated model: %s", ex.what());
    }

    rollbackModelToEarlierState();
}

void osc::UndoableUiModel::forciblyRollbackToEarlierState() {
    rollbackModelToEarlierState();
}

void osc::UndoableUiModel::clearAnyDamagedModels() {
    if (m_Damaged) {
        log::error("destructing damaged model");
        m_Damaged = std::nullopt;
    }
}
