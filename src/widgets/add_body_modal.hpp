#pragma once

namespace OpenSim {
    class PhysicalFrame;
    class Model;
    class Component;
}

namespace osmv {
    template<typename T>
    class Indirect_ref;
    template<typename T>
    class Indirect_ptr;
}

namespace osmv {
    struct Added_body_modal_state {
        static constexpr char const* modal_name = "add body";

        OpenSim::PhysicalFrame const* selected_pf = nullptr;
        char body_name[64]{};
        char joint_name[64]{};
        float mass = 1.0f;
        float com[3]{};
        float inertia[3] = {1.0f, 1.0f, 1.0f};
        bool add_offset_frames_to_the_joint = true;
        bool inertia_locked = true;
        bool com_locked = true;
    };

    void show_add_body_modal(Added_body_modal_state&);
    void try_draw_add_body_modal(
        Added_body_modal_state&, Indirect_ref<OpenSim::Model>&, Indirect_ptr<OpenSim::Component>& selection);
}
