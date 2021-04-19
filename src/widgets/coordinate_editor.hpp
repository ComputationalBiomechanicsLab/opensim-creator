#pragma once

#include <vector>

namespace OpenSim {
    class Coordinate;
    class Model;
}

namespace SimTK {
    class State;
}

namespace osc {
    struct Coordinate_editor_state final {
        char filter[64]{};
        bool sort_by_name = true;
        bool show_rotational = true;
        bool show_translational = true;
        bool show_coupled = true;
        std::vector<OpenSim::Coordinate const*> coord_scratch;
    };

    void get_coordinates(OpenSim::Model const&, std::vector<OpenSim::Coordinate const*>&);

    // returns `true` if `State` was edited by the coordinate editor
    bool draw_coordinate_editor(Coordinate_editor_state&, OpenSim::Model const&, SimTK::State&);
}
