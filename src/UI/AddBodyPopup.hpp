#pragma once

#include "src/Assertions.hpp"
#include "src/UI/AttachGeometryPopup.hpp"

#include <memory>
#include <optional>

namespace OpenSim {
    class PhysicalFrame;
    class Model;
    class Mesh;
    class Body;
    class Joint;
}

namespace osc {
    struct NewBody final {
        std::unique_ptr<OpenSim::Body> body;
        std::unique_ptr<OpenSim::Joint> joint;

        NewBody(std::unique_ptr<OpenSim::Body> b,
                 std::unique_ptr<OpenSim::Joint> j) :
            body{std::move(b)},
            joint{std::move(j)} {

            OSC_ASSERT(body != nullptr);
            OSC_ASSERT(joint != nullptr);
        }
    };

    struct AddBodyPopup final {
        // sub-modal for attaching geometry to the body
        struct {
            AttachGeometryPopup state;
            std::unique_ptr<OpenSim::Geometry> selected = nullptr;
        } attach_geom;

        OpenSim::PhysicalFrame const* selectedPF = nullptr;

        char bodyName[64]{};
        int jointIdx = 0;
        char jointName[64]{};
        float mass = 1.0f;
        float com[3]{};
        float inertia[3] = {1.0f, 1.0f, 1.0f};
        bool addOffsetFramesToTheJoint = true;
        bool inertiaLocked = true;
        bool comLocked = true;

        // assumes caller has handled calling ImGui::OpenPopup(modal_name)
        std::optional<NewBody> draw(char const* modalName, OpenSim::Model const&);
    };
}
