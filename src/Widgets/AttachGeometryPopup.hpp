#pragma once

#include <memory>

namespace OpenSim
{
    class Geometry;
}

namespace osc
{
    class AttachGeometryPopup final {
    public:
        AttachGeometryPopup();
        AttachGeometryPopup(AttachGeometryPopup const&) = delete;
        AttachGeometryPopup(AttachGeometryPopup&&) noexcept;
        AttachGeometryPopup& operator=(AttachGeometryPopup const&) = delete;
        AttachGeometryPopup& operator=(AttachGeometryPopup&&) noexcept;
        ~AttachGeometryPopup() noexcept;

        // this assumes caller handles calling ImGui::OpenPopup(modal_name)
        //
        // returns non-nullptr when user selects an OpenSim::Mesh
        std::unique_ptr<OpenSim::Geometry> draw(char const* modalName);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
