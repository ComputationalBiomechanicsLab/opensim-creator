#pragma once

namespace OpenSim {
    class Component;
}

namespace osmv {
    template<typename T>
    class Indirect_ptr;
}

namespace osmv {
    class Component_hierarchy_widget final {
    public:
        Component_hierarchy_widget() = default;
        Component_hierarchy_widget(Component_hierarchy_widget const&) = delete;
        Component_hierarchy_widget(Component_hierarchy_widget&&) = delete;
        Component_hierarchy_widget& operator=(Component_hierarchy_widget const&) = delete;
        Component_hierarchy_widget& operator=(Component_hierarchy_widget&&) = delete;
        ~Component_hierarchy_widget() noexcept = default;

        // the caller should provide the root component that should be rendered
        // (i.e. as a tree below the root) and pointers to pointers to the selection
        // and hover. E.g.:
        //
        //     OpenSim::Model const* model = get_model();  // model inherits from Component
        //     OpenSim::Component const* prev_selection = get_selection();
        //     OpenSim::Component const* selection = prev_selection;
        //     OpenSim::Component const* hover = nullptr;  // you can provide nullptr to either
        //
        //     widget.draw(model, &selection, &hover);
        //
        //     if (hover) {
        //         // the widget set a new hover
        //     }
        //
        //     if (selection != prev_selection) {
        //         // the widget changed the selection
        //     }
        //
        void draw(
            OpenSim::Component const* root,
            Indirect_ptr<OpenSim::Component>& selection,
            Indirect_ptr<OpenSim::Component>& hover);
    };
}
