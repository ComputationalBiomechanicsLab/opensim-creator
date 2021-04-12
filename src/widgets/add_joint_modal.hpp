#pragma once

#include <functional>
#include <memory>
#include <string>

namespace OpenSim {
    class Model;
    class Component;
    class Joint;
    class PhysicalFrame;
}

namespace osc {
    struct Add_joint_modal final {
        static constexpr char default_name[] = "new_joint";

        std::string modal_name;
        std::unique_ptr<OpenSim::Joint> joint_prototype;
        OpenSim::PhysicalFrame const* parent_frame = nullptr;
        OpenSim::PhysicalFrame const* child_frame = nullptr;
        char added_joint_name[128]{};

        template<typename T>
        static Add_joint_modal create(std::string name) {
            return Add_joint_modal{std::move(name), std::make_unique<T>()};
        }

        Add_joint_modal(std::string _name, std::unique_ptr<OpenSim::Joint> _prototype);

        void reset();
        void show();
        void draw(OpenSim::Model const&, std::function<void(std::unique_ptr<OpenSim::Joint>)> const& on_add);
    };
}
