#include "LOGLHelloTriangleTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/Rgba32.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/HelloTriangle";

    osc::Mesh GenerateTriangleMesh()
    {
        constexpr auto points = osc::to_array<glm::vec3>(
        {
            {-1.0f, -1.0f, 0.0f},  // bottom-left
            { 1.0f, -1.0f, 0.0f},  // bottom-right
            { 0.0f,  1.0f, 0.0f},  // top-middle
        });

        // care: we're using colors that are equivalent in sRGB and linear
        //       color spaces here
        constexpr auto colors = osc::to_array<osc::Rgba32>(
        {
            {0xff, 0x00, 0x00, 0xff},
            {0x00, 0xff, 0x00, 0xff},
            {0x00, 0x00, 0xff, 0xff},
        });

        constexpr auto indices = osc::to_array<uint16_t>({0, 1, 2});

        osc::Mesh m;
        m.setVerts(points);
        m.setColors(colors);
        m.setIndices(indices);
        return m;
    }

    osc::Camera CreateSceneCamera()
    {
        osc::Camera rv;
        rv.setViewMatrixOverride(glm::mat4{1.0f});
        rv.setProjectionMatrixOverride(glm::mat4{1.0f});
        return rv;
    }

    osc::Material CreateTriangleMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("shaders/ExperimentTriangle.vert"),
                osc::App::slurp("shaders/ExperimentTriangle.frag"),
            },
        };
    }
}

class osc::LOGLHelloTriangleTab::Impl final : public osc::StandardTabBase {
public:

    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnDraw() final
    {
        Graphics::DrawMesh(m_TriangleMesh, osc::Transform{}, m_Material, m_Camera);

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

    Material m_Material = CreateTriangleMaterial();
    Mesh m_TriangleMesh = GenerateTriangleMesh();
    Camera m_Camera = CreateSceneCamera();
};


// public API

osc::CStringView osc::LOGLHelloTriangleTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLHelloTriangleTab::LOGLHelloTriangleTab(LOGLHelloTriangleTab&&) noexcept = default;
osc::LOGLHelloTriangleTab& osc::LOGLHelloTriangleTab::operator=(LOGLHelloTriangleTab&&) noexcept = default;
osc::LOGLHelloTriangleTab::~LOGLHelloTriangleTab() noexcept = default;

osc::UID osc::LOGLHelloTriangleTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLHelloTriangleTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLHelloTriangleTab::implOnDraw()
{
    m_Impl->onDraw();
}
