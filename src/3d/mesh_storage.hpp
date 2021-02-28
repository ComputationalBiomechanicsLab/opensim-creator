#pragma once

#include "src/3d/gpu_data_reference.hpp"

namespace osmv {
    struct Mesh_on_gpu;
    struct Untextured_vert;
    struct Textured_vert;
    struct Plain_mesh;
    struct Textured_mesh;
}

namespace osmv {
    class Mesh_storage final {
        struct Impl;
        Impl* impl;

    public:
        Mesh_storage();
        Mesh_storage(Mesh_storage const&) = delete;
        Mesh_storage(Mesh_storage&&) = delete;
        Mesh_storage& operator=(Mesh_storage const&) = delete;
        Mesh_storage& operator=(Mesh_storage&&) = delete;
        ~Mesh_storage() noexcept;

        [[nodiscard]] Mesh_on_gpu& lookup(Mesh_reference) const;

        [[nodiscard]] Mesh_reference allocate(Plain_mesh const&);
        [[nodiscard]] Mesh_reference allocate(Textured_mesh const&);

        template<typename Container>
        [[nodiscard]] Mesh_reference allocate(Container const& c) {
            return allocate(c.data(), c.size());
        }
    };
}
