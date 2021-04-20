#pragma once

#include "src/assertions.hpp"
#include "src/ui/attach_geometry_popup.hpp"

#include <memory>
#include <optional>

namespace OpenSim {
    class PhysicalFrame;
    class Model;
    class Mesh;
    class Body;
    class Joint;
}

namespace osc::widgets::add_body_popup {
    struct State {
        struct {
            attach_geometry_popup::State state;
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

    struct New_body final {
        std::unique_ptr<OpenSim::Body> body;
        std::unique_ptr<OpenSim::Joint> joint;

        New_body(std::unique_ptr<OpenSim::Body> b, std::unique_ptr<OpenSim::Joint> j) :
            body{std::move(b)},
            joint{std::move(j)} {

            OSC_ASSERT(body != nullptr);
            OSC_ASSERT(joint != nullptr);
        }
    };

    // assumes caller has handled calling ImGui::OpenPopup(modal_name)
    std::optional<New_body> draw(State&, char const* modal_name, OpenSim::Model const&);
}
