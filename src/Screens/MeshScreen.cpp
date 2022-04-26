#include "MeshScreen.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"

#include <imgui.h>
#include <glm/vec2.hpp>

#include <vector>

class osc::MeshScreen::Impl final {
public:
    void onMount()
    {
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::cur().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;  // ImGui handled this particular event
        }
    }

    void tick(float)
    {
        UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), m_Camera);
    }

    void draw()
    {
        // called once per frame. Code in here should use drawing primitives, OpenGL, ImGui,
        // etc. to draw things into the screen. The application does not clear the screen
        // buffer between frames (it's assumed that your code does this when it needs to)

        osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // set app window bg color
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear app window with bg color

        gl::UseProgram(m_Shader.program);
        gl::Uniform(m_Shader.uDiffuseColor, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
        gl::Uniform(m_Shader.uModelMat, gl::identity);
        gl::Uniform(m_Shader.uNormalMat, glm::mat3x3{1.0f});
        gl::Uniform(m_Shader.uViewMat, m_Camera.getViewMtx());
        gl::Uniform(m_Shader.uProjMat, m_Camera.getProjMtx(App::cur().aspectRatio()));
        gl::Uniform(m_Shader.uIsTextured, true);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::Uniform(m_Shader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
        gl::Uniform(m_Shader.uLightColor, {1.0f, 1.0f, 1.0f});
        gl::Uniform(m_Shader.uLightDir, {-0.34f, +0.25f, 0.05f});
        gl::Uniform(m_Shader.uViewPos, m_Camera.getPos());

        //gl::Disable(GL_CULL_FACE);
        gl::BindVertexArray(m_Mesh.GetVertexArray());
        m_Mesh.Draw();
        gl::BindVertexArray();

        ImGui::Begin("cookiecutter panel");

        {
            Line const& ray = m_Camera.unprojectTopLeftPosToWorldRay(App::cur().getMouseState().pos, App::cur().dims());
            if (m_Mesh.getClosestRayTriangleCollisionModelspace(ray))
            {
                ImGui::Text("hit");
            }
        }

        auto tcs = m_Mesh.getTexCoords();
        std::vector<glm::vec2> texCoords;
        for (glm::vec2 const& tc : tcs)
        {
            texCoords.push_back(tc*1.001f);
        }
        m_Mesh.setTexCoords(texCoords);

        ImGui::Text("hello world");
        ImGui::Checkbox("checkbox_state", &m_CheckboxState);

        osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
    }

private:
    bool m_CheckboxState = false;
    GouraudShader m_Shader;
    Mesh m_Mesh{GenTexturedQuad()};
    gl::Texture2D m_Chequer = genChequeredFloorTexture();
    PolarPerspectiveCamera m_Camera;
};


// public API (PIMPL)

osc::MeshScreen::MeshScreen() :
    m_Impl{new Impl{}}
{
}

osc::MeshScreen::MeshScreen(MeshScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshScreen& osc::MeshScreen::operator=(MeshScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MeshScreen::~MeshScreen() noexcept
{
    delete m_Impl;
}

void osc::MeshScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MeshScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MeshScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MeshScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::MeshScreen::draw()
{
    m_Impl->draw();
}
