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
    class Gpu_data_reference {
    public:
        using value_type = T;
        static constexpr value_type invalid_value = -1;

    private:
        value_type id;

    public:
        constexpr Gpu_data_reference(decltype(id) _id) noexcept : id{_id} {
        }

        [[nodiscard]] constexpr bool is_valid() const noexcept {
            // the use of a negative senteniel interplays with sort logic very well,
            // because it ensures (for example) that invalid references cluster at
            // the start of a sequence, not (e.g.) in the middle
            static_assert(invalid_value < 0);
            return id >= 0;
        }

        constexpr operator bool() const noexcept {
            return is_valid();
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

        [[nodiscard]] constexpr size_t to_index() const noexcept {
            assert(is_valid());
            return static_cast<size_t>(id);
        }
    };

    class Mesh_reference : public Gpu_data_reference<short> {
    public:
        [[nodiscard]] static constexpr Mesh_reference from_index(size_t idx) noexcept {
            assert(idx < std::numeric_limits<value_type>::max());
            return {static_cast<value_type>(idx)};
        }

        [[nodiscard]] static constexpr Mesh_reference invalid() noexcept {
            return Mesh_reference{invalid_value};
        }
    };

    class Texture_reference : public Gpu_data_reference<short> {
    public:
        [[nodiscard]] static constexpr Texture_reference from_index(size_t idx) noexcept {
            assert(idx < std::numeric_limits<value_type>::max());
            return {static_cast<value_type>(idx)};
        }

        [[nodiscard]] static constexpr Texture_reference invalid() noexcept {
            return Texture_reference{invalid_value};
        }
    };
}
