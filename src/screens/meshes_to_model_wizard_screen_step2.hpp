#pragma once

#include "src/screens/screen.hpp"
#include "src/3d/3d.hpp"

#include <memory>
#include <filesystem>

namespace osc {

    // what the initial screen should produce as an output
    struct Loaded_user_mesh final {
        // location of the mesh file on disk
        std::filesystem::path location;

        // raw vertex/element data
        Untextured_mesh meshdata;

        // AABB (modelspace) bounding box of meshdata
        AABB aabb;

        // bounding sphere (modelspace) for meshdata
        Sphere bounding_sphere;

        // index of meshdata on GPU
        Meshidx gpu_meshidx;

        // true if the mesh is hovered
        bool is_hovered;

        // true if the mesh is selected
        bool is_selected;
    };

    class Meshes_to_model_wizard_screen_step2 final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Meshes_to_model_wizard_screen_step2(std::vector<Loaded_user_mesh>);
        ~Meshes_to_model_wizard_screen_step2() noexcept override;

        void draw() override;
        void tick(float) override;
    };
}
