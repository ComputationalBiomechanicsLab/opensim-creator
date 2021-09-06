#pragma once

#include "src/ui/properties_editor.hpp"
#include "src/Assertions.hpp"

#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>
#include <vector>

namespace OpenSim {
    class Model;
    class AbstractSocket;
    class PhysicalFrame;
}

namespace osc {
    std::vector<OpenSim::AbstractSocket const*> getPhysframeSockets(OpenSim::Component& c);

    struct AddComponentPopup final {
        // a prototypical version of the component being added
        //
        // this is what the user edits *before* adding it into the model
        std::unique_ptr<OpenSim::Component> prototype;

        std::string name;

        // a property editor for the prototype
        //
        // lets user edit prototype's properties in-place
        osc::ui::properties_editor::State propEditor;

        // cached sequence of OpenSim::PhysicalFrame sockets in `prototype`
        std::vector<OpenSim::AbstractSocket const*> pfSockets;

        // connectees in the `OpenSim::Model` that the user selected for each
        // socket in `prototype`
        //
        // the UX assumes all PhysicalFrame sockets will be assigned a connectee
        std::vector<OpenSim::PhysicalFrame const*> pfConnectees;

        // a sequence of PhysicalFrames that should be used as path points
        //
        // this is what the user adds path points to - if the prototype contains path points
        struct PathPoint final {
            // a user-selected path point
            OpenSim::PhysicalFrame const* ptr = nullptr;

            // bookkeeping: the implementation also checks that `ptr` is still present
            // in the provided `model` argument
            bool missing = false;
        };
        std::vector<PathPoint> pps;


        AddComponentPopup(std::unique_ptr<OpenSim::Component> prototype);

        // - assumes caller handles ImGui::OpenPopup(modal_name)
        // - returns nullptr until the user fully builds the Component
        std::unique_ptr<OpenSim::Component> draw(char const* modal_name, OpenSim::Model const&);
    };
}
