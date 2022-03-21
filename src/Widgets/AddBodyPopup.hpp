#pragma once

#include <memory>
#include <optional>

namespace OpenSim
{
    class PhysicalFrame;
    class Model;
    class Mesh;
    class Body;
    class Joint;
}

namespace osc
{
    struct NewBody final {
        std::unique_ptr<OpenSim::Body> body;
        std::unique_ptr<OpenSim::Joint> joint;

        NewBody(std::unique_ptr<OpenSim::Body>,
                std::unique_ptr<OpenSim::Joint>);
    };

    class AddBodyPopup final {
    public:
        AddBodyPopup();
        AddBodyPopup(AddBodyPopup const&) = delete;
        AddBodyPopup(AddBodyPopup&&) noexcept;
        AddBodyPopup& operator=(AddBodyPopup const&) = delete;
        AddBodyPopup& operator=(AddBodyPopup&&) noexcept;
        ~AddBodyPopup() noexcept;

        // assumes caller has handled calling ImGui::OpenPopup(modal_name)
        std::optional<NewBody> draw(char const* modalName, OpenSim::Model const&);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
