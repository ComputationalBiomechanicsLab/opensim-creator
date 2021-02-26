#pragma once

namespace OpenSim {
    class Component;
    class ModelDisplayHints;
}

namespace SimTK {
    class State;
}

namespace osmv {
    struct Gpu_cache;
    class Model_drawlist;
}

namespace osmv {
    void generate_decoration_drawlist(
        OpenSim::Component const&, SimTK::State const&, OpenSim::ModelDisplayHints const&, Gpu_cache&, Model_drawlist&);
}
