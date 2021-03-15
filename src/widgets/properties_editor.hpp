#pragma once

#include <vector>

namespace OpenSim {
    class Component;
}

namespace osmv {
    template<typename T>
    class Indirect_ptr;
}

namespace osmv {
    class Properties_editor final {
        std::vector<bool> property_locked;

    public:
        bool draw(Indirect_ptr<OpenSim::Component>&);
    };
}
