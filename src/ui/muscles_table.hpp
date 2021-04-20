#pragma once

#include <limits>
#include <vector>

namespace OpenSim {
    class Muscle;
    class Model;
    class Component;
}

namespace SimTK {
    class State;
}

namespace osc::widgets::muscles_table {
    using SortChoice = int;
    enum SortChoice_ { SortChoice_Length = 0, SortChoice_Strain, SortChoice_NUM_CHOICES };

#define OSC_MUSCLE_SORT_NAMES                                                                                          \
    { "length", "strain" }

    struct State final {
        char filter[64]{};
        float min_len = std::numeric_limits<float>::min();
        float max_len = std::numeric_limits<float>::max();
        std::vector<OpenSim::Muscle const*> muscles;
        SortChoice sort_choice = SortChoice_Length;
        bool inverse_range = false;
        bool reverse_results = false;
    };

    enum Response_type {
        NothingChanged,
        HoverChanged,
        SelectionChanged,
    };

    struct Response final {
        Response_type type = NothingChanged;
        OpenSim::Muscle const* ptr = nullptr;
    };

    Response draw(State&, OpenSim::Model const&, SimTK::State const&);
}
