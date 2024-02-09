#include "LOGLHelloTriangleTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/HelloTriangle";

    Mesh GenerateTriangleMesh()
    {
        Mesh m;
        m.setVerts({{
            {-1.0f, -1.0f, 0.0f},  // bottom-left
            { 1.0f, -1.0f, 0.0f},  // bottom-right
            { 0.0f,  1.0f, 0.0f},  // top-middle
        }});
        m.setColors({{
            Color::red(),
            Color::green(),
            Color::blue(),
        }});
        m.setIndices(std::to_array<uint16_t>({0, 1, 2}));
        return m;
    }

    Camera CreateSceneCamera()
    {
        Camera rv;
        rv.setViewMatrixOverride(Identity<Mat4>());
        rv.setProjectionMatrixOverride(Identity<Mat4>());
        return rv;
    }

    Material CreateTriangleMaterial(IResourceLoader& rl)
    {
        return Material{Shader{
            rl.slurp("oscar_learnopengl/shaders/GettingStarted/HelloTriangle.vert"),
            rl.slurp("oscar_learnopengl/shaders/GettingStarted/HelloTriangle.frag"),
        }};
    }
}

class osc::LOGLHelloTriangleTab::Impl final : public StandardTabImpl {
public:

    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnDraw() final
    {
        Graphics::DrawMesh(m_TriangleMesh, Identity<Transform>(), m_Material, m_Camera);

        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

    ResourceLoader m_Loader = App::resource_loader();
    Material m_Material = CreateTriangleMaterial(m_Loader);
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    Camera m_Camera = CreateSceneCamera();
};


// public API

CStringView osc::LOGLHelloTriangleTab::id()
{
    return c_TabStringID;
}

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(LOGLHelloTriangleTab&&) noexcept = default;
osc::LOGLHelloTriangleTab& osc::LOGLHelloTriangleTab::operator=(LOGLHelloTriangleTab&&) noexcept = default;
osc::LOGLHelloTriangleTab::~LOGLHelloTriangleTab() noexcept = default;

UID osc::LOGLHelloTriangleTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLHelloTriangleTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLHelloTriangleTab::implOnDraw()
{
    m_Impl->onDraw();
}
