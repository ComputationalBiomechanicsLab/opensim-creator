#pragma once

namespace osmv {
using DrawlistGeneratorFlags = int;
enum DrawlistGeneratorFlags_ {
    DrawlistGeneratorFlags_None = 0,
    DrawlistGeneratorFlags_GenerateDynamicDecorations = 1 << 0,
    DrawlistGeneratorFlags_GenerateStaticDecorations = 1 << 1,
    DrawlistGeneratorFlags_Default =
        DrawlistGeneratorFlags_GenerateDynamicDecorations | DrawlistGeneratorFlags_GenerateStaticDecorations,
};

// used to generate a raw drawlist from an OpenSim Model + State
struct Drawlist_generator_impl;
class Drawlist_generator final {
    Drawlist_generator_impl* impl;

public:
    Drawlist_generator();
    Drawlist_generator(Drawlist_generator const&) = delete;
    Drawlist_generator(Drawlist_generator&&) = delete;
    Drawlist_generator& operator=(Drawlist_generator const&) = delete;
    Drawlist_generator& operator=(Drawlist_generator&&) = delete;
    ~Drawlist_generator() noexcept;

    void generate(
        OpenSim::Model const&,
        SimTK::State const&,
        OpenSim_model_geometry& out,
        std::function<void(OpenSim::Component const*&, Mesh_instance&)> on_append = [](auto&, auto&) {},
        DrawlistGeneratorFlags flags = DrawlistGeneratorFlags_Default);
};
}
