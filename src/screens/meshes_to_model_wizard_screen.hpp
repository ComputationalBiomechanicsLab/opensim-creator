#pragma once

#include "src/screens/screen.hpp"

#include <memory>

namespace osc {
    // shows a wizard for importing mesh files (e.g. VTPs, OBJs) and mapping
    // them into a new osim model
    //
    // useful for when a user has a bunch of anatomical meshes--e.g. from a
    // scanner, MRI, whatever--and the user wants to just map those meshes
    // onto a basic OpenSim model
    class Meshes_to_model_wizard_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Meshes_to_model_wizard_screen();
        ~Meshes_to_model_wizard_screen() noexcept override;

        void draw() override;
    };
}
