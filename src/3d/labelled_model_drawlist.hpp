#pragma once

#include "raw_renderer.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace OpenSim {
    class Component;
}

namespace osmv {
    // geometry generated from an OpenSim model + SimTK state pair
    struct OpenSim_model_geometry final {
        // these two vectors are 1:1 associated
        std::vector<Mesh_instance> instances;
        std::vector<OpenSim::Component const*> associated_components;

        void clear() {
            instances.clear();
            associated_components.clear();
        }

        template<typename... Args>
        Mesh_instance& emplace_back(OpenSim::Component const* c, Args&&... args) {
            size_t idx = associated_components.size();

            if (idx >= (1 << 16)) {
                throw std::runtime_error{
                    "precondition error: tried to render more than the maximum number of components osmv can render"};
            }

            associated_components.push_back(c);
            Mesh_instance& rv = instances.emplace_back(std::forward<Args>(args)...);

            // encode index+1 into the passthrough data, so that:
            //
            // - mesh instances can be re-ordered and still know which component they
            //   are associated with
            //
            // - the renderer can pass through which component (index) is associated
            //   with a screen pixel, but callers can reassign the *components* to other
            //   components (the *index* is encoded, not the component)
            //
            // must be >0 (so idx+1), because zeroed passthrough data implies "no information",
            // rather than "information, which is zero"
            rv.set_passthrough_data(Passthrough_data::from_u16(idx + 1));

            return rv;
        }

        OpenSim::Component const* component_from_passthrough(Passthrough_data d) {
            uint16_t id = d.to_u16();
            return id == 0 ? nullptr : associated_components[id - 1];
        }

        template<typename Callback>
        void for_each(Callback f) {
            // emplace-back ensures this
            assert(instances.size() == associated_components.size());

            for (Mesh_instance& mi : instances) {
                uint16_t id = mi.passthrough_data().to_u16();
                assert(id != 0);  // emplace-back ensures this
                f(associated_components[id - 1], mi);
            }
        }
    };
}
