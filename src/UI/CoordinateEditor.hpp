#pragma once

#include "src/OpenSimBindings/UiModel.hpp"

#include <vector>

namespace OpenSim
{
    class Coordinate;
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc {
    void GetCoordinatesInModel(OpenSim::Model const&, std::vector<OpenSim::Coordinate const*>&);
}

namespace osc {
    struct CoordinateEditor final {
        char filter[64]{};
        bool sort_by_name = true;
        bool show_rotational = true;
        bool show_translational = true;
        bool show_coupled = true;
        std::vector<OpenSim::Coordinate const*> coord_scratch;

        // returns `true` if `State` was edited by the coordinate editor
        bool draw(UiModel&);
    };
}
