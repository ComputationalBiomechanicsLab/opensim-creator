#pragma once

#include <vector>

namespace OpenSim {
    class Component;
}

namespace osmv {
    class Properties_editor final {
        std::vector<bool> property_locked;

    public:
        bool draw(OpenSim::Component&);
    };
}
