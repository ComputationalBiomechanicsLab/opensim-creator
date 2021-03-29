#pragma once

#include <OpenSim/Common/Component.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace osmv {
    class Component_path_ptrs {
        static constexpr size_t max_component_depth = 16;
        using container = std::array<OpenSim::Component const*, max_component_depth>;

        container els{};
        size_t n;

    public:
        explicit Component_path_ptrs(OpenSim::Component const& c) : n{0} {
            OpenSim::Component const* cp = &c;

            els[n++] = cp;
            while (cp->hasOwner()) {
                if (n >= max_component_depth) {
                    throw std::runtime_error{
                        "cannot traverse hierarchy to a component: it is deeper than 32 levels in the component tree, which isn't currently supported by osmv"};
                }

                cp = &cp->getOwner();
                els[n++] = cp;
            }
            std::reverse(els.begin(), els.begin() + n);
        }

        [[nodiscard]] constexpr container::const_iterator begin() const noexcept {
            return els.begin();
        }

        [[nodiscard]] constexpr container::const_iterator end() const noexcept {
            return els.begin() + n;
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return n == 0;
        }
    };

    inline Component_path_ptrs path_to(OpenSim::Component const& c) {
        return Component_path_ptrs{c};
    }
}
