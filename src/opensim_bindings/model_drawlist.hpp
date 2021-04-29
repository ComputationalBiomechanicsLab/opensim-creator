#pragma once

#include "src/3d/3d.hpp"
#include "src/assertions.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace OpenSim {
    class Component;
}

namespace osc {

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

        void push_back(OpenSim::Component const* c, Mesh_instance const& mi) {
            size_t idx = associated_components.size();

            if (idx >= std::numeric_limits<uint16_t>::max()) {
                throw std::runtime_error{
                    "precondition error: tried to render more than the maximum number of components osc can render"};
            }


            uint16_t passthrough_id = static_cast<uint16_t>(idx + 1);

            Mesh_instance copy = mi;
            copy.passthrough.b0 = static_cast<GLubyte>(passthrough_id);
            copy.passthrough.b1 = static_cast<GLubyte>(passthrough_id >> 8);

            associated_components.emplace_back(c);
            drawlist.push_back(copy);
        }

        [[nodiscard]] static constexpr uint16_t decode_le_u16(GLubyte b0, GLubyte b1) noexcept {
            uint16_t rv = static_cast<uint16_t>(b0);
            rv |= static_cast<uint16_t>(b1) << 8;
            return rv;
        }

        OpenSim::Component const* component_from_passthrough(Rgb24 d) {
            static_assert(sizeof(d.r) == 1);
            static_assert(sizeof(d.g) == 1);

            uint16_t data = decode_le_u16(d.r, d.g);

            if (data > 0) {
                return associated_components[data - 1];
            } else {
                return nullptr;
            }
        }

        template<typename Callback>
        void for_each(Callback f) {
            // emplace-back ensures this
            OSC_ASSERT(drawlist.size() == associated_components.size());

            drawlist.for_each([&](Mesh_instance& mi) {
                uint16_t id = decode_le_u16(mi.passthrough.b0, mi.passthrough.b1);
                OSC_ASSERT(id != 0 && "zero ID inserted into drawlist (emplace_back should prevent this)");
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

        Drawlist& raw_drawlist() noexcept {
            return drawlist;
        }
    };

    inline void optimize(Model_drawlist& mdl) noexcept {
        optimize(mdl.drawlist);
    }
}
