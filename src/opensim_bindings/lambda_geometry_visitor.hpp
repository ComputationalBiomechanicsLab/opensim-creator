#pragma once

#include "src/opensim_bindings/simbody_geometry_visitor.hpp"

#include <utility>

namespace osmv {
    struct Mesh_instance;
}

namespace osmv {
    template<typename OnInstanceCreatedCallback>
    class Lambda_geometry_visitor final : public Simbody_geometry_visitor {
        OnInstanceCreatedCallback callback;

    public:
        template<typename... BaseCtorArgs>
        Lambda_geometry_visitor(OnInstanceCreatedCallback _callback, BaseCtorArgs&&... args) :
            Simbody_geometry_visitor{std::forward<BaseCtorArgs>(args)...},
            callback(std::move(_callback)) {
        }

    private:
        void on_instance_created(Mesh_instance const& mi) override {
            callback(mi);
        }
    };
}
