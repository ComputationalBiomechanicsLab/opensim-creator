#pragma once

#include <cassert>
#include <cstddef>
#include <limits>

namespace osmv {
    // a soft, non-owned, reference to a mesh
    //
    // users of this class are expected to know the actual lifetime of the mesh
    // being referenced, because this class does not come with automatic cleanup
    // guarantees (it's designed to be trivially constructable/copyable/movable
    // in-memory)
    template<typename T>
    class Gpu_data_reference final {
        using meshid_t = T;
        static constexpr meshid_t senteniel = -1;
        static constexpr meshid_t max_meshid = std::numeric_limits<meshid_t>::max();

        meshid_t id;

    public:
        static constexpr Gpu_data_reference invalid() noexcept {
            return Gpu_data_reference{senteniel};
        }

        static constexpr Gpu_data_reference from_index(size_t idx) {
            assert(idx < max_meshid);
            return Gpu_data_reference{static_cast<meshid_t>(idx)};
        }

        // trivial constructability might be important for high-perf algs
        //
        // user beware ;)
        Gpu_data_reference() = default;

        constexpr Gpu_data_reference(decltype(id) _id) noexcept : id{_id} {
        }

        constexpr bool is_valid() const noexcept {
            // the use of a negative senteniel interplays with sort logic very well,
            // because it ensures (for example) that invalid references cluster at
            // the start of a sequence, not (e.g.) in the middle
            static_assert(senteniel < 0);
            return id >= 0;
        }

        constexpr bool operator==(Gpu_data_reference const& other) const noexcept {
            return id == other.id;
        }

        constexpr bool operator!=(Gpu_data_reference const& other) const noexcept {
            return id != other.id;
        }

        constexpr bool operator<(Gpu_data_reference const& other) const noexcept {
            return id < other.id;
        }

        constexpr size_t to_index() const {
            assert(is_valid());
            return static_cast<size_t>(id);
        }
    };

    using Mesh_reference = Gpu_data_reference<short>;
}
