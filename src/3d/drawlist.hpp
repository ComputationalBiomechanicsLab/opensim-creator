#pragma once

#include "src/3d/mesh_instance.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace osmv {
    class Drawlist final {
        friend class Renderer;

        std::vector<Mesh_instance> instances;

    public:
        [[nodiscard]] size_t size() const noexcept {
            return instances.size();
        }

        template<typename... Args>
        Mesh_instance& emplace_back(Args... args) {
            return instances.emplace_back(std::forward<Args>(args)...);
        }

        void clear() {
            instances.clear();
        }

        template<typename Callback>
        void for_each(Callback f) {
            for (Mesh_instance& mi : instances) {
                f(mi);
            }
        }

        // permitted to re-order or minorly mutate elements, but not remove any
        //
        // not permitted to modify Raw_mesh_instance::passthrough_data - use that to encode
        // any information you need *before* optimizing
        void optimize() noexcept;
    };
}
