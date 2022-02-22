#pragma once

namespace OpenSim {
    class Component;
}

namespace osc {
    class ComponentHierarchy final {
        char search[256]{};
        bool showFrames = false;

    public:
        enum ResponseType { NothingHappened, SelectionChanged, HoverChanged };
        struct Response final {
            OpenSim::Component const* ptr = nullptr;
            ResponseType type = NothingHappened;
        };

        Response draw(
            OpenSim::Component const* root,
            OpenSim::Component const* selection,
            OpenSim::Component const* hover);
    };
}
