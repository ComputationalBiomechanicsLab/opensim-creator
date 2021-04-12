#pragma once

namespace OpenSim {
    class Component;
    class ModelDisplayHints;
}

namespace SimTK {
    class State;
}

namespace osc {
    struct Gpu_cache;
    class Model_drawlist;
}

namespace osc {
    using ModelDrawlistFlags = int;
    enum ModelDrawlistFlags_ {
        ModelDrawlistFlags_None = 0,
        ModelDrawlistFlags_StaticGeometry = 1 << 0,
        ModelDrawlistFlags_DynamicGeometry = 1 << 2,
        ModelDrawlistFlags_Default = ModelDrawlistFlags_StaticGeometry | ModelDrawlistFlags_DynamicGeometry,
    };

    void generate_decoration_drawlist(
        OpenSim::Component const&,
        SimTK::State const&,
        OpenSim::ModelDisplayHints const&,
        Gpu_cache&,
        Model_drawlist&,
        ModelDrawlistFlags = ModelDrawlistFlags_Default);
}
