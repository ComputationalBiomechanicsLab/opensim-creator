#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {

    class Tut1_hellotriangle_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Tut1_hellotriangle_screen();
        ~Tut1_hellotriangle_screen() noexcept;

        void tick(float) override;
        void draw() override;
    };
}
