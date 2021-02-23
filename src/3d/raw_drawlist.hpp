#pragma once

#include "mesh_reference.hpp"
#include "raw_mesh_instance.hpp"

#include <vector>

namespace osmv {

    // a raw list of instances that should be drawn by the renderer
    //
    // you might ask: "why not just use a plain vector?". Good idea - for now. But the future
    // intention is to start to use zero-overhead memory mapping for instance data (e.g.
    // memory-mapped buffers /w unsynchronized access patterns + memory fenches, round-robin
    // flipping, etc.)
    //
    // so this tries to shield downstream from that. You can emplace things into this
    // list, optimize it, and mutate it by iterating over it (/w for_each) one-by-one. You
    // can't have random access, etc. because GPU optimizations might require rearranging
    // things in memory quite radically (e.g. instead of optimizing by sorting, use separate
    // memory arenas for each mesh, etc. etc.), and you can't resize it (again, memory arenas)
    // but you can "clear" it (which might, in the future, actually mean "flip between
    // unsynchronized memory-mapped buffers")
    class Raw_drawlist final {
        friend class Raw_renderer;

        std::vector<Raw_mesh_instance> instances;

    public:
        size_t size() const noexcept {
            return instances.size();
        }

        template<typename... Args>
        Raw_mesh_instance& emplace_back(Args... args) {
            return instances.emplace_back(std::forward<Args>(args)...);
        }

        void clear() {
            instances.clear();
        }

        template<typename Callback>
        void for_each(Callback f) {
            for (Raw_mesh_instance& mi : instances) {
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
