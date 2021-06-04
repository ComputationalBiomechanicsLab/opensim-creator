#pragma once

#include "src/ui/properties_editor.hpp"
#include "src/assertions.hpp"

#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>
#include <vector>

namespace OpenSim {
    class Model;
    class AbstractSocket;
    class PhysicalFrame;
}

namespace osc::ui::add_component_popup {
    std::vector<OpenSim::AbstractSocket const*> get_pf_sockets(OpenSim::Component& c);

    struct State final {
        // a prototypical version of the component being added
        //
        // this is what the user edits *before* adding it into the model
        std::unique_ptr<OpenSim::Component> prototype;

        std::string name;

        // a property editor for the prototype
        //
        // lets user edit prototype's properties in-place
        properties_editor::State prop_editor;

        // cached sequence of OpenSim::PhysicalFrame sockets in `prototype`
        std::vector<OpenSim::AbstractSocket const*> pf_sockets;

        // connectees in the `OpenSim::Model` that the user selected for each
        // socket in `prototype`
        //
        // the UX assumes all PhysicalFrame sockets will be assigned a connectee
        std::vector<OpenSim::PhysicalFrame const*> pf_connectees;

        // a sequence of PhysicalFrames that should be used as path points
        //
        // this is what the user adds path points to - if the prototype contains path points
        struct Path_point final {
            // a user-selected path point
            OpenSim::PhysicalFrame const* ptr = nullptr;

            // bookkeeping: the implementation also checks that `ptr` is still present
            // in the provided `model` argument
            bool missing = false;
        };
        std::vector<Path_point> pps;


        State(std::unique_ptr<OpenSim::Component> _prototype) :
            prototype{std::move(_prototype)},
            name{prototype->getConcreteClassName()},
            prop_editor{},
            pf_sockets{get_pf_sockets(*prototype)},
            pf_connectees(pf_sockets.size()),
            pps{} {

            OSC_ASSERT(pf_sockets.size() == physframe_connectee_choices.size());
        }
    };

    // - assumes caller handles ImGui::OpenPopup(modal_name)
    // - returns nullptr until the user fully builds the Component
    std::unique_ptr<OpenSim::Component> draw(State&, char const* modal_name, OpenSim::Model const&);
}
