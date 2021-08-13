#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    class Tut2_mesh_instance_meshdata_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Tut2_mesh_instance_meshdata_screen();
        ~Tut2_mesh_instance_meshdata_screen() noexcept;

        void draw() override;
    };
}
