#pragma once

#include "gpu_data_reference.hpp"

#include <memory>

namespace osmv {
    struct Mesh_on_gpu;
    struct Untextured_vert;
    struct Textured_vert;
}

namespace osmv {
    class Mesh_storage final {
        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        Mesh_storage();
        ~Mesh_storage() noexcept;

        Mesh_on_gpu& lookup(Mesh_reference) const;

        Mesh_reference allocate(Untextured_vert const* verts, size_t n);
        Mesh_reference allocate(Textured_vert const* verts, size_t n);

        template<typename Container>
        Mesh_reference allocate(Container const& c) {
            return allocate(c.data(), c.size());
        }
    };
}
