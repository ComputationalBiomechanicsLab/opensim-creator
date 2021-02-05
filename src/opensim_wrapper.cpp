#include "opensim_wrapper.hpp"

#include "config.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <SimTKcommon.h>

#include <utility>

osmv::Model::Model(std::unique_ptr<OpenSim::Model> _m) noexcept : handle{std::move(_m)} {
}

osmv::Model::Model(Model const& m) : Model{static_cast<OpenSim::Model const&>(m)} {
}

osmv::Model::Model(std::filesystem::path const& p) : handle{std::make_unique<OpenSim::Model>(p.string())} {
}
osmv::Model::Model(OpenSim::Model const& m) : handle{new OpenSim::Model{m}} {
}
osmv::Model::Model(Model&&) noexcept = default;
osmv::Model& osmv::Model::operator=(Model&&) noexcept = default;
osmv::Model::~Model() noexcept = default;

osmv::State::State(SimTK::State const& st) : handle{new SimTK::State(st)} {
}
osmv::State::State(std::unique_ptr<SimTK::State> _handle) noexcept : handle{std::move(_handle)} {
}
osmv::State& osmv::State::operator=(SimTK::State const& st) {
    if (handle != nullptr) {
        *handle = st;
    } else {
        handle = std::make_unique<SimTK::State>(st);
    }
    return *this;
}
osmv::State& osmv::State::operator=(std::unique_ptr<SimTK::State> _handle) noexcept {
    handle = std::move(_handle);
    return *this;
}
osmv::State::State(State&&) noexcept = default;
osmv::State& osmv::State::operator=(State&&) noexcept = default;
osmv::State::~State() noexcept = default;
