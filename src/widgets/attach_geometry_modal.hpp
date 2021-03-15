#pragma once

#include <filesystem>
#include <vector>

namespace OpenSim {
    class Model;
    class PhysicalFrame;
}

namespace osmv {
    template<typename T>
    class Indirect_ptr;
    class Snapshot_taker;
}

namespace osmv {
    struct Attach_geometry_modal final {
        static constexpr char const* name = "attach geometry";

        std::vector<std::filesystem::path> recent;
        char search[128]{};

        void show();
        void draw(std::vector<std::filesystem::path> const& vtps, Indirect_ptr<OpenSim::PhysicalFrame>&);
    };
}
