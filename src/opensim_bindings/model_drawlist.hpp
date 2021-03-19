#pragma once

#include "src/3d/drawlist.hpp"
#include "src/assertions.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace OpenSim {
    class Component;
}

namespace osmv {

    // geometry generated from an OpenSim model + SimTK state pair
    class Model_drawlist final {
        friend void optimize(Model_drawlist&) noexcept;

        // these two are 1:1 associated
        Drawlist drawlist;
        std::vector<OpenSim::Component const*> associated_components;

    public:
        void clear() {
            drawlist.clear();
            associated_components.clear();
        }

        template<typename... Args>
        Mesh_instance& emplace_back(OpenSim::Component const* c, Args&&... args) {
            size_t idx = associated_components.size();

            if (idx >= std::numeric_limits<uint16_t>::max()) {
                throw std::runtime_error{
                    "precondition error: tried to render more than the maximum number of components osmv can render"};
            }

            // this is safe because of the above assert
            uint16_t passthrough_id = static_cast<uint16_t>(idx + 1);

            associated_components.emplace_back(c);
            Mesh_instance& mesh_instance = drawlist.emplace_back(std::forward<Args>(args)...);

            // encode index+1 into the passthrough data, so that:
            //
            // - mesh instances can be re-ordered (e.g. for draw call optimization) and
            //   still know which component they are associated with
            //
            // - the renderer can pass through which component (index) is associated
            //   with a screen pixel, but callers can reassign the *components* to other
            //   components (the *index* is encoded, not the component)
            //
            // must be >0 (so idx+1), because zeroed passthrough data implies "no information",
            // rather than "information, which is zero"
            mesh_instance.set_passthrough_data(Passthrough_data::from_u16(passthrough_id));

            return mesh_instance;
        }

        OpenSim::Component const* component_from_passthrough(Passthrough_data d) {
            uint16_t id = d.to_u16();
            return id == 0 ? nullptr : associated_components[id - 1];
        }

        template<typename Callback>
        void for_each(Callback f) {
            // emplace-back ensures this
            OSMV_ASSERT(drawlist.size() == associated_components.size());

            drawlist.for_each([&](Mesh_instance& mi) {
                uint16_t id = mi.passthrough_data().to_u16();
                OSMV_ASSERT(id != 0 && "zero ID inserted into drawlist (emplace_back should prevent this)");
                f(associated_components[id - 1], mi);
            });
        }

        template<typename Callback>
        void for_each_component(Callback f) {
            for (OpenSim::Component const*& c : associated_components) {
                f(c);
            }
        }

        Drawlist const& raw_drawlist() const noexcept {
            return drawlist;
        }
    };

    inline void optimize(Model_drawlist& mdl) noexcept {
        optimize(mdl.drawlist);
    }
}
