#pragma once

#include "src/screen.hpp"

#include <memory>
#include <vector>
#include <filesystem>

namespace osc {
    // meshes-to-model wizard
    //
    // a screen that helps users import 3D meshes into a new OpenSim model.
    // This is a separate screen from the main UI because it involves letting
    // the user manipulate meshes/bodies/joints in free/ground 3D space *before*
    // committing to OpenSim's constraints
    class Meshes_to_model_wizard_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        // shows blank scene that a user can import meshes into
        Meshes_to_model_wizard_screen();

        // shows the blank scene, but immediately starts importing the provided
        // mesh filepaths
        Meshes_to_model_wizard_screen(std::vector<std::filesystem::path>);

        ~Meshes_to_model_wizard_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void draw() override;
        void tick(float) override;
    };
}
