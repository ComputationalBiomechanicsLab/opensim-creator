#include "MeshScreen.hpp"

#include "src/3D/Shaders/GouraudShader.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Mesh.hpp"
#include "src/3D/Texturing.hpp"
#include "src/Utils/SimTKHelpers.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/Perf.hpp"
#include "src/App.hpp"

#include <imgui.h>

#include <iostream>

struct osc::MeshScreen::Impl final {
    bool checkboxState = false;
    GouraudShader shader;
    //Mesh m{SimTKLoadMesh("/home/adam/Desktop/OpenSim_Model_Files/Geometry/Thorax_Pelvis_PectoralGirdle.obj")};
    Mesh m{GenTexturedQuad()};
    gl::Texture2D chequer = genChequeredFloorTexture();
    PolarPerspectiveCamera camera;
};

// public API

osc::MeshScreen::MeshScreen() :
    m_Impl{new Impl{}} {
}

osc::MeshScreen::~MeshScreen() noexcept = default;

void osc::MeshScreen::onMount() {
    // called when app receives the screen, but before it starts pumping events
    // into it, ticking it, drawing it, etc.

    osc::ImGuiInit();  // boot up ImGui support
}

void osc::MeshScreen::onUnmount() {
    // called when the app is going to stop pumping events/ticks/draws into this
    // screen (e.g. because the app is quitting, or transitioning to some other screen)

    osc::ImGuiShutdown();  // shutdown ImGui support
}

void osc::MeshScreen::onEvent(SDL_Event const& e) {
    // called when the app receives an event from the operating system

    // pump event into ImGui, if using it:
    if (osc::ImGuiOnEvent(e)) {
        return;  // ImGui handled this particular event
    }
}

void osc::MeshScreen::tick(float) {
    // called once per frame, before drawing, with a timedelta from the last call
    // to `tick`

    // use this if you need to regularly update something (e.g. an animation, or
    // file polling)

    UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), m_Impl->camera);
}

void osc::MeshScreen::draw() {
    // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
    // etc. to draw things into the screen. The application does not clear the screen
    // buffer between frames (it's assumed that your code does this when it needs to)

    osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // set app window bg color
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear app window with bg color

    Impl& st = *m_Impl;

    gl::UseProgram(st.shader.program);
    gl::Uniform(st.shader.uDiffuseColor, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
    gl::Uniform(st.shader.uModelMat, gl::identity);
    gl::Uniform(st.shader.uNormalMat, glm::mat3x3{1.0f});
    gl::Uniform(st.shader.uViewMat, st.camera.getViewMtx());
    gl::Uniform(st.shader.uProjMat, st.camera.getProjMtx(App::cur().aspectRatio()));
    gl::Uniform(st.shader.uIsTextured, true);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::Uniform(st.shader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
    gl::Uniform(st.shader.uLightColor, {1.0f, 1.0f, 1.0f});
    gl::Uniform(st.shader.uLightDir, {-0.34f, +0.25f, 0.05f});
    gl::Uniform(st.shader.uViewPos, st.camera.getPos());

    //gl::Disable(GL_CULL_FACE);
    gl::BindVertexArray(st.m.GetVertexArray());
    st.m.Draw();
    gl::BindVertexArray();

    ImGui::Begin("cookiecutter panel");

    {
        Line const& ray = st.camera.unprojectTopLeftPosToWorldRay(App::cur().getMouseState().pos, App::cur().dims());
        if (st.m.getClosestRayTriangleCollisionModelspace(ray)) {
            ImGui::Text("hit");
        }
    }

    auto tcs = st.m.getTexCoords();
    std::vector<glm::vec2> texCoords;
    for (glm::vec2 const& tc : tcs) {
        texCoords.push_back(tc*1.001f);
    }
    st.m.setTexCoords(texCoords);



    ImGui::Text("hello world");
    ImGui::Checkbox("checkbox_state", &m_Impl->checkboxState);

    osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
}
