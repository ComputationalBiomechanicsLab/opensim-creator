#pragma once

#include <functional>
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

namespace osc {
    using MusclesTableSortChoice = int;
    enum MusclesTableSortChoice_ {
        MusclesTableSortChoice_Length = 0,
        MusclesTableSortChoice_Strain,
        MusclesTableSortChoice_NUM_CHOICES
    };

#define OSC_MUSCLE_SORT_NAMES                                                                                         \
    { "length", "strain" }

    struct Muscles_table_state final {
        char filter[64]{};
        float min_len = std::numeric_limits<float>::min();
        float max_len = std::numeric_limits<float>::max();
        std::vector<OpenSim::Muscle const*> muscles;
        MusclesTableSortChoice sort_choice = MusclesTableSortChoice_Length;
        bool inverse_range = false;
        bool reverse_results = false;
    };

    void draw_muscles_table(
        Muscles_table_state&,
        OpenSim::Model const&,
        SimTK::State const&,
        std::function<void(OpenSim::Component const*)> const& on_hover,
        std::function<void(OpenSim::Component const*)> const& on_select);
}
