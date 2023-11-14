#include "LOGLHelloTriangleTab.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <array>
#include <cstdint>
#include <memory>

using osc::Mat4;
using osc::Vec3;

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/HelloTriangle";

    osc::Mesh GenerateTriangleMesh()
    {
        constexpr auto points = std::to_array<Vec3>(
        {
            {-1.0f, -1.0f, 0.0f},  // bottom-left
            { 1.0f, -1.0f, 0.0f},  // bottom-right
            { 0.0f,  1.0f, 0.0f},  // top-middle
        });

        // care: we're using colors that are equivalent in sRGB and linear
        //       color spaces here
        constexpr auto colors = std::to_array<osc::Color>(
        {
            osc::Color::red(),
            osc::Color::green(),
            osc::Color::blue(),
        });

        constexpr auto indices = std::to_array<uint16_t>({0, 1, 2});

        osc::Mesh m;
        m.setVerts(points);
        m.setColors(colors);
        m.setIndices(indices);
        return m;
    }

    osc::Camera CreateSceneCamera()
    {
        osc::Camera rv;
        rv.setViewMatrixOverride(osc::Identity<Mat4>());
        rv.setProjectionMatrixOverride(osc::Identity<Mat4>());
        return rv;
    }

    osc::Material CreateTriangleMaterial()
    {
        return osc::Material
        {
            osc::Shader
            {
                osc::App::slurp("oscar_learnopengl/shaders/GettingStarted/HelloTriangle.vert"),
                osc::App::slurp("oscar_learnopengl/shaders/GettingStarted/HelloTriangle.frag"),
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

osc::CStringView osc::LOGLHelloTriangleTab::id()
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
