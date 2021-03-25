#pragma once

#include "src/widgets/attach_geometry_modal.hpp"

#include <functional>
#include <memory>

namespace OpenSim {
    class PhysicalFrame;
    class Model;
    class Mesh;
    class Body;
    class Joint;
}

namespace osmv {
    struct Added_body_modal_state {
        struct {
            Attach_geometry_modal_state state;
            std::unique_ptr<OpenSim::Mesh> selected = nullptr;
        } attach_geom;

        OpenSim::PhysicalFrame const* selected_pf = nullptr;

        char body_name[64]{};
        int joint_idx = 0;
        char joint_name[64]{};
        float mass = 1.0f;
        float com[3]{};
        float inertia[3] = {1.0f, 1.0f, 1.0f};
        bool add_offset_frames_to_the_joint = true;
        bool inertia_locked = true;
        bool com_locked = true;
    };

    struct Added_body_modal_output final {
        std::unique_ptr<OpenSim::Body> body;
        std::unique_ptr<OpenSim::Joint> joint;

        Added_body_modal_output(std::unique_ptr<OpenSim::Body> b, std::unique_ptr<OpenSim::Joint> j) :
            body{std::move(b)},
            joint{std::move(j)} {
        }
    };

    // assumes caller has handled calling ImGui::OpenPopup(modal_name)
    void try_draw_add_body_modal(
        Added_body_modal_state&,
        char const* modal_name,
        OpenSim::Model const&,
        std::function<void(Added_body_modal_output)> const& on_add_requested);
}
