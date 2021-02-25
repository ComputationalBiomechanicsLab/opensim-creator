#pragma once

#include "gpu_data_reference.hpp"

#include <memory>

namespace gl {
    struct Texture_2d;
}

namespace osmv {
    class Texture_storage final {
        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        Texture_storage();
        ~Texture_storage() noexcept;

        gl::Texture_2d& lookup(Texture_reference) const;
        Texture_reference allocate(gl::Texture_2d&&);
    };
}
