#pragma once

#include "src/Screen.hpp"

#include <memory>
#include <vector>
#include <filesystem>

namespace osc {
    // meshes-to-model wizard
    //
    // A screen that helps users import 3D meshes into a new OpenSim model.
    //
    // This is a separate screen from the main UI because it involves letting
    // the user manipulate meshes/bodies/joints in free/ground 3D space, much
    // like if they were using Blender, *before* trying to stuff everything
    // into an OpenSim::Model (which has constraints, relative coordinates,
    // etc.)
    class MeshesToModelWizardScreen final : public Screen {
    public:
        struct Impl;
    private:
        Impl* m_Impl;

    public:
        // shows blank scene that a user can import meshes into
        MeshesToModelWizardScreen();

        // shows the blank scene, but immediately starts importing the provided
        // mesh filepaths
        MeshesToModelWizardScreen(std::vector<std::filesystem::path>);

        ~MeshesToModelWizardScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;
        void tick(float) override;
    };
}
