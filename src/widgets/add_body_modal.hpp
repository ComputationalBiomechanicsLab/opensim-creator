#pragma once

namespace OpenSim {
    class Model;
    class Component;
    class PhysicalFrame;
}

namespace osmv {
    struct Added_body_modal_state {
        OpenSim::PhysicalFrame const* selected_pf = nullptr;
        char body_name[64]{};
        char joint_name[64]{};
        float mass = 1.0f;
        float com[3]{};
        float inertia[3]{};
        bool add_offset_frames_to_the_joint = true;
    };

    void show_add_body_modal(Added_body_modal_state&);
    void try_draw_add_body_modal(Added_body_modal_state&, OpenSim::Model&, OpenSim::Component const**);
}
