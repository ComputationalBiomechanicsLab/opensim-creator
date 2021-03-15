#pragma once

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Component;
}

namespace osmv {
    template<typename T>
    class Indirect_ptr;
}

namespace osmv {
    class Component_selection_widget final {
    public:
        Component_selection_widget() = default;
        Component_selection_widget(Component_selection_widget const&) = delete;
        Component_selection_widget(Component_selection_widget&&) = delete;
        Component_selection_widget& operator=(Component_selection_widget const&) = delete;
        Component_selection_widget& operator=(Component_selection_widget&&) = delete;
        ~Component_selection_widget() noexcept = default;

        // caller should provide a pointer to a pointer to a component
        //
        // e.g.:
        //     Component const* previous = get_selection();
        //     Component const* selected = previous;
        //
        //     widget.draw(state, &selected);
        //
        //     if (selected != previous) {
        //         // selection changed
        //     }
        //
        // this is because the user can change the selection inside this widget
        // (e.g. by hopping up to the parent component)
        void draw(SimTK::State const& state, Indirect_ptr<OpenSim::Component>&);
    };
}
