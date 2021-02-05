#pragma once

#include <filesystem>
#include <memory>

namespace OpenSim {
    class Model;
}

namespace SimTK {
    class State;
}

// opensim_wrapper: wrapper code for OpenSim
//
// Main motivation for this header is to compiler-firewall OpenSim away from
// the rest of the codebase because OpenSim.h is massive and can increase
// compile times by *a lot*
namespace osmv {

    // owned (but opaque) handle to an OpenSim::Model
    struct Model final {
        std::unique_ptr<OpenSim::Model> handle;

        explicit Model(std::unique_ptr<OpenSim::Model>) noexcept;
        explicit Model(OpenSim::Model const&);
        explicit Model(Model const& m);
        explicit Model(std::filesystem::path const&);
        Model(Model&&) noexcept;
        Model& operator=(Model const&) = delete;
        Model& operator=(Model&&) noexcept;
        ~Model() noexcept;

        operator OpenSim::Model const&() const noexcept {
            return *handle;
        }

        operator OpenSim::Model&() noexcept {
            return *handle;
        }

        OpenSim::Model* operator->() noexcept {
            return handle.get();
        }

        OpenSim::Model const* operator->() const noexcept {
            return handle.get();
        }

        OpenSim::Model const* get() const noexcept {
            return handle.get();
        }

        OpenSim::Model* get() noexcept {
            return handle.get();
        }
    };

    // owned (but opaque) handle to a SimTK::State
    struct State final {
        std::unique_ptr<SimTK::State> handle;

        explicit State(SimTK::State const&);
        explicit State(State const& s) : State{static_cast<SimTK::State const&>(s)} {
        }
        explicit State(std::unique_ptr<SimTK::State>) noexcept;
        State(State&&) noexcept;
        State& operator=(State const&) = delete;
        State& operator=(SimTK::State const&);
        State& operator=(std::unique_ptr<SimTK::State>) noexcept;
        State& operator=(State&&) noexcept;
        ~State() noexcept;

        operator SimTK::State const&() const noexcept {
            return *handle;
        }

        operator SimTK::State&() noexcept {
            return *handle;
        }

        SimTK::State* operator->() noexcept {
            return handle.get();
        }

        SimTK::State const* operator->() const noexcept {
            return handle.get();
        }
    };
}
