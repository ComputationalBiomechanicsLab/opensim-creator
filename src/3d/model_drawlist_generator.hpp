#pragma once

#include "labelled_model_drawlist.hpp"

#include <functional>

namespace OpenSim {
    class Model;
}

namespace SimTK {
    class State;
}

namespace osmv {

    using ModelDrawlistGeneratorFlags = int;
    enum ModelDrawlistGeneratorFlags_ {
        ModelDrawlistGeneratorFlags_None = 0,
        ModelDrawlistGeneratorFlags_GenerateDynamicDecorations = 1 << 0,
        ModelDrawlistGeneratorFlags_GenerateStaticDecorations = 1 << 1,
        ModelDrawlistGeneratorFlags_Default = ModelDrawlistGeneratorFlags_GenerateDynamicDecorations |
                                              ModelDrawlistGeneratorFlags_GenerateStaticDecorations,
    };

    using ModelDrawlistOnAppendFlags = int;
    enum ModelDrawlistOnAppendFlags_ {
        ModelDrawlistOnAppendFlags_None = 0,
        ModelDrawlistOnAppendFlags_IsStatic = 1 << 0,
        ModelDrawlistOnAppendFlags_IsDynamic = 1 << 1,
    };

    // used to generate a raw drawlist from an OpenSim Model + State
    struct Drawlist_generator_impl;
    class Model_drawlist_generator final {
        Drawlist_generator_impl* impl;

    public:
        Model_drawlist_generator();
        Model_drawlist_generator(Model_drawlist_generator const&) = delete;
        Model_drawlist_generator(Model_drawlist_generator&&) = delete;
        Model_drawlist_generator& operator=(Model_drawlist_generator const&) = delete;
        Model_drawlist_generator& operator=(Model_drawlist_generator&&) = delete;
        ~Model_drawlist_generator() noexcept;

        void generate(
            OpenSim::Model const&,
            SimTK::State const&,
            Labelled_model_drawlist& out,
            std::function<void(ModelDrawlistOnAppendFlags, OpenSim::Component const*&, Raw_mesh_instance&)> on_append =
                [](auto, auto&, auto&) {},
            ModelDrawlistGeneratorFlags flags = ModelDrawlistGeneratorFlags_Default);
    };
}
