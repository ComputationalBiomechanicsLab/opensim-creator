#include "Component3DViewerScreen.hpp"

#include "src/App.hpp"
#include "src/3d/Gl.hpp"
#include "src/screens/experimental/ExperimentsScreen.hpp"
#include "src/ui/Component3DViewer.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

struct osc::Component3DViewerScreen::Impl final {
    OpenSim::Model model{App::resource("models/RajagopalModel/Rajagopal2015.osim").string()};
    //OpenSim::Model model{App::resource("models/ToyLanding/ToyLandingModel.osim").string()};
    SimTK::State state = [this]() {
        model.finalizeFromProperties();
        model.finalizeConnections();
        SimTK::State s = model.initSystem();
        model.realizeReport(s);
        return s;
    }();
    Component3DViewer viewer;
    OpenSim::Component const* hovered = nullptr;
};

// public API

osc::Component3DViewerScreen::Component3DViewerScreen() :
    m_Impl{new Impl{}} {
}

osc::Component3DViewerScreen::~Component3DViewerScreen() noexcept = default;

void osc::Component3DViewerScreen::onMount() {
    osc::ImGuiInit();
}

void osc::Component3DViewerScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::Component3DViewerScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
    }
}

void osc::Component3DViewerScreen::draw() {
    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto res = m_Impl->viewer.draw("viewer", m_Impl->model, m_Impl->state, nullptr, m_Impl->hovered);
    if (res.hovertestResult) {
        m_Impl->hovered = res.hovertestResult;
    }

    osc::ImGuiRender();
}
