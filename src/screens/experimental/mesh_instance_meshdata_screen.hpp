#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    class Mesh_instance_meshdata_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Mesh_instance_meshdata_screen();
        ~Mesh_instance_meshdata_screen() noexcept;

        void draw() override;
    };
}
