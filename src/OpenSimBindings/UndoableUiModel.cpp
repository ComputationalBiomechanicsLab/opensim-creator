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

osc::UndoableUiModel::UndoableUiModel() :
    m_Current{},
    m_Backup{m_Current},
    m_UndoBuffer{},
    m_RedoBuffer{} {
}

osc::UndoableUiModel::UndoableUiModel(std::unique_ptr<OpenSim::Model> model) :
    m_Current{std::move(model)},
    m_Backup{m_Current},
    m_UndoBuffer{},
    m_RedoBuffer{} {
}

UiModel const& osc::UndoableUiModel::getUiModel() const {
    return m_Current;
}

UiModel& osc::UndoableUiModel::updUiModel() {
    return m_Current;
}

bool osc::UndoableUiModel::canUndo() const noexcept {
    return !m_UndoBuffer.empty();
}

void osc::UndoableUiModel::doUndo() {
    if (!canUndo()) {
        return;
    }

    // ensure the backup has equivalent pointers to the current
    m_Backup.setSelectedHoveredAndIsolatedFrom(m_Current);

    // push backup onto the redo buffer (it's guaranteed to be non-dirty)
    m_RedoBuffer.push_back(std::move(m_Backup));

    // pop undo onto current
    m_Current = m_UndoBuffer.pop_back();

    // migrate pointers from backup to current, to update the undo's selection-state
    m_Current.setSelectedHoveredAndIsolatedFrom(m_RedoBuffer.back());

    // copy new current, which should be fine at this point
    m_Backup = m_Current;
}

bool osc::UndoableUiModel::canRedo() const noexcept {
    return !m_RedoBuffer.empty();
}

void osc::UndoableUiModel::doRedo() {
    if (!canRedo()) {
        return;
    }

    // ensure the backup has equivalent pointers to current
    m_Backup.setSelectedHoveredAndIsolatedFrom(m_Current);

    // push backup onto undo buffer (it's guaranteed to be non-dirty)
    m_UndoBuffer.push_back(std::move(m_Backup));

    // pop redo onto current
    m_Current = m_RedoBuffer.pop_back();

    // migrate pointers from backup to current, to update the selection state
    m_Current.setSelectedHoveredAndIsolatedFrom(m_UndoBuffer.back());

    // copy new current
    m_Backup = m_Current;
}

OpenSim::Model const& osc::UndoableUiModel::getModel() const noexcept {
    return m_Current.getModel();
}

OpenSim::Model& osc::UndoableUiModel::updModel() noexcept {
    return m_Current.updModel();
}

void osc::UndoableUiModel::setModel(std::unique_ptr<OpenSim::Model> newModel) {
    // ensure any current changes are applied + backed up before trying
    // to initialize from the incoming model
    updateIfDirty();

    // initialize from the incoming model, rolling back if there's an issue
    try {
        m_Current.setModel(std::move(newModel));
        m_Current.setSelectedHoveredAndIsolatedFrom(m_Backup);
        m_Current.updateIfDirty();
        m_Backup = m_Current;
    } catch (std::exception const& ex) {
        log::error("exception thrown while updating the current model from an external (probably, file-loaded) model");
        log::error("%s", ex.what());
        log::error("attempting to rollback to an earlier version of the model");
        m_Current = m_Backup;
    }
}

SimTK::State const& osc::UndoableUiModel::getState() const noexcept {
    return m_Current.getState();
}

SimTK::State& osc::UndoableUiModel::updState() noexcept {
    return m_Current.updState();
}

float osc::UndoableUiModel::getFixupScaleFactor() const {
    return m_Current.getFixupScaleFactor();
}

void osc::UndoableUiModel::setFixupScaleFactor(float v) {
    m_Current.setFixupScaleFactor(v);
}

float osc::UndoableUiModel::getReccommendedScaleFactor() const {
    return m_Current.getRecommendedScaleFactor();
}

void osc::UndoableUiModel::updateIfDirty() {
    if (!m_Current.isDirty()) {
        return;
    }

    try {
        m_Current.updateIfDirty();
    } catch (std::exception const& ex) {
        log::error("exception occurred after applying changes to a model:");
        log::error("%s", ex.what());
        log::error("attempting to rollback to an earlier version of the model");
        m_Current = m_Backup;
    }

    // copy backup into undo buffer if the latest entry in the undo buffer is >5s old
    if (m_UndoBuffer.empty() || (m_UndoBuffer.back().getLastModifiedTime() < std::chrono::system_clock::now() - 5s)) {
        m_UndoBuffer.push_back(m_Backup);
    }

    // copy current into backup, so there's a "clean" copy hanging around
    m_Backup = m_Current;
}

void osc::UndoableUiModel::setModelDirty(bool v) {
    setAllDirty(v);
}

void osc::UndoableUiModel::setStateDirty(bool v) {
    setAllDirty(v);
}

void osc::UndoableUiModel::setDecorationsDirty(bool v) {
    setAllDirty(v);
}

void osc::UndoableUiModel::setAllDirty(bool v) {
    m_Current.setAllDirty(v);
}

bool osc::UndoableUiModel::hasSelected() const {
    return m_Current.hasSelected();
}

OpenSim::Component const* osc::UndoableUiModel::getSelected() const {
    return m_Current.getSelected();
}

OpenSim::Component* osc::UndoableUiModel::updSelected() {
    return m_Current.updSelected();
}

void osc::UndoableUiModel::setSelected(OpenSim::Component const* c) {
    m_Current.setSelected(c);
}

bool osc::UndoableUiModel::selectionHasTypeHashCode(size_t v) const {
    return m_Current.selectionHasTypeHashCode(v);
}

bool osc::UndoableUiModel::hasHovered() const {
    return m_Current.hasHovered();
}

OpenSim::Component const* osc::UndoableUiModel::getHovered() const noexcept {
    return m_Current.getHovered();
}

OpenSim::Component* osc::UndoableUiModel::updHovered() {
    return m_Current.updHovered();
}

void osc::UndoableUiModel::setHovered(OpenSim::Component const* c) {
    m_Current.setHovered(c);
}

OpenSim::Component const* osc::UndoableUiModel::getIsolated() const noexcept {
    return m_Current.getIsolated();
}

OpenSim::Component* osc::UndoableUiModel::updIsolated() {
    return m_Current.updIsolated();
}

void osc::UndoableUiModel::setIsolated(OpenSim::Component const* c) {
    m_Current.setIsolated(c);
}

void osc::UndoableUiModel::declareDeathOf(const OpenSim::Component *c) noexcept {
    if (m_Current.getSelected() == c) {
        m_Current.setSelected(nullptr);
    }

    if (m_Current.getHovered() == c) {
        m_Current.setHovered(nullptr);
    }

    if (m_Current.getIsolated() == c) {
        m_Current.setIsolated(nullptr);
    }
}
