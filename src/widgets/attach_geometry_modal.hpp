#pragma once

#include <filesystem>
#include <memory>
#include <vector>

namespace OpenSim {
    class Model;
    class PhysicalFrame;
}

namespace osmv {
    struct Attach_geometry_modal final {
        static constexpr char const* name = "attach geometry";

        std::vector<std::filesystem::path> recent;
        char search[128]{};

        void show();
        void draw(
            OpenSim::Model& model,
            std::vector<std::filesystem::path> const& vtps,
            std::vector<std::unique_ptr<OpenSim::Model>>& snapshots,
            OpenSim::PhysicalFrame& frame);
    };
}
