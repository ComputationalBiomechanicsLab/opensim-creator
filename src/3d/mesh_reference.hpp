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
    class Mesh_reference final {
        using meshid_t = int;
        static constexpr meshid_t senteniel = -1;
        static constexpr meshid_t max_meshid = std::numeric_limits<meshid_t>::max();

        meshid_t id;

    public:
        static constexpr Mesh_reference invalid() noexcept {
            return Mesh_reference{senteniel};
        }

        static constexpr Mesh_reference from_index(size_t idx) {
            assert(idx < max_meshid);
            return Mesh_reference{static_cast<meshid_t>(idx)};
        }

        // trivial constructability might be important for high-perf algs
        //
        // user beware ;)
        Mesh_reference() = default;

        constexpr Mesh_reference(decltype(id) _id) noexcept : id{_id} {
        }

        constexpr bool is_valid() const noexcept {
            static_assert(senteniel < 0);
            return id >= 0;
        }

        constexpr bool operator==(Mesh_reference const& other) const noexcept {
            return id == other.id;
        }

        constexpr bool operator!=(Mesh_reference const& other) const noexcept {
            return id != other.id;
        }

        constexpr bool operator<(Mesh_reference const& other) const noexcept {
            return id < other.id;
        }

        constexpr size_t to_index() const {
            assert(is_valid());
            return static_cast<size_t>(id);
        }
    };
}
