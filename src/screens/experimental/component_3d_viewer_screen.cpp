#include "component_3d_viewer_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/ui/component_3d_viewer.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

struct osc::Component_3d_viewer_screen::Impl final {
    OpenSim::Model model{App::resource("models/RajagopalModel/Rajagopal2015.osim").string()};
    //OpenSim::Model model{App::resource("models/ToyLanding/ToyLandingModel.osim").string()};
    SimTK::State state = [this]() {
        model.finalizeFromProperties();
        model.finalizeConnections();
        SimTK::State s = model.initSystem();
        model.realizeReport(s);
        return s;
    }();
    Component_3d_viewer viewer;
    OpenSim::Component const* hovered = nullptr;
};

// public API

osc::Component_3d_viewer_screen::Component_3d_viewer_screen() :
    m_Impl{new Impl{}} {
}

osc::Component_3d_viewer_screen::~Component_3d_viewer_screen() noexcept = default;

void osc::Component_3d_viewer_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Component_3d_viewer_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Component_3d_viewer_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }
}

void osc::Component_3d_viewer_screen::draw() {
    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto res = m_Impl->viewer.draw("viewer", m_Impl->model, m_Impl->state, nullptr, m_Impl->hovered);
    if (res.hovertest_result) {
        m_Impl->hovered = res.hovertest_result;
    }

    osc::ImGuiRender();
}
