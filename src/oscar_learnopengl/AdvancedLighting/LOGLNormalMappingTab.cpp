#include "LOGLNormalMappingTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <SDL_events.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

using osc::App;
using osc::Camera;
using osc::ColorSpace;
using osc::CStringView;
using osc::Material;
using osc::Mesh;
using osc::MeshIndicesView;
using osc::MeshTopology;
using osc::Shader;
using osc::Texture2D;
using osc::Vec2;
using osc::Vec3;
using osc::Vec4;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/NormalMapping";

    // matches the quad used in LearnOpenGL's normal mapping tutorial
    Mesh GenerateQuad()
    {
        auto const verts = std::to_array<Vec3>(
        {
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        });
        auto const normals = std::to_array<Vec3>(
        {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        });
        auto const texCoords = std::to_array<Vec2>(
        {
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        });
        auto const indices = std::to_array<uint16_t>(
        {
            0, 1, 2,
            0, 2, 3,
        });

        std::vector<Vec4> const tangents = osc::CalcTangentVectors(
            MeshTopology::Triangles,
            verts,
            normals,
            texCoords,
            MeshIndicesView{indices}
        );
        OSC_ASSERT_ALWAYS(tangents.size() == verts.size());

        Mesh rv;
        rv.setVerts(verts);
        rv.setNormals(normals);
        rv.setTexCoords(texCoords);
        rv.setTangents(tangents);
        rv.setIndices(indices);
        return rv;
    }

    Camera CreateCamera()
    {
        Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(osc::Deg2Rad(45.0f));
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    Material CreateNormalMappingMaterial()
    {
        Texture2D diffuseMap = osc::LoadTexture2DFromImage(
            App::resource("oscar_learnopengl/textures/brickwall.jpg"),
            ColorSpace::sRGB
        );
        Texture2D normalMap = osc::LoadTexture2DFromImage(
            App::resource("oscar_learnopengl/textures/brickwall_normal.jpg"),
            ColorSpace::Linear
        );

        Material rv
        {
            Shader
            {
                App::slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.vert"),
                App::slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.frag"),
            },
        };
        rv.setTexture("uDiffuseMap", diffuseMap);
        rv.setTexture("uNormalMap", normalMap);

        return rv;
    }

    Material CreateLightCubeMaterial()
    {
        return Material
        {
            Shader
            {
                App::slurp("oscar_learnopengl/shaders/LightCube.vert"),
                App::slurp("oscar_learnopengl/shaders/LightCube.frag"),
            },
        };
    }
}

class osc::LOGLNormalMappingTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
        m_LightTransform.position = {0.5f, 1.0f, 0.3f};
        m_LightTransform.scale *= 0.2f;
    }

private:
    void implOnMount() final
    {
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        // handle mouse capturing
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void implOnTick() final
    {
        // rotate the quad over time
        AppClock::duration const dt = App::get().getFrameDeltaSinceAppStartup();
        double const angle = Deg2Rad(-10.0 * dt.count());
        Vec3 const axis = Normalize(Vec3{1.0f, 0.0f, 1.0f});
        m_QuadTransform.rotation = Normalize(Quat{static_cast<float>(angle), axis});
    }

    void implOnDraw() final
    {
        // handle mouse capturing and update camera
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // clear screen and ensure camera has correct pixel rect
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw normal-mapped quad
        {
            m_NormalMappingMaterial.setVec3("uLightWorldPos", m_LightTransform.position);
            m_NormalMappingMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());
            m_NormalMappingMaterial.setBool("uEnableNormalMapping", m_IsNormalMappingEnabled);
            Graphics::DrawMesh(m_QuadMesh, m_QuadTransform, m_NormalMappingMaterial, m_Camera);
        }

        // draw light source cube
        {
            m_LightCubeMaterial.setColor("uLightColor", Color::white());
            Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);
        }

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        ImGui::Begin("controls");
        ImGui::Checkbox("normal mapping", &m_IsNormalMappingEnabled);
        ImGui::End();
    }

    // rendering state
    Material m_NormalMappingMaterial = CreateNormalMappingMaterial();
    Material m_LightCubeMaterial = CreateLightCubeMaterial();
    Mesh m_CubeMesh = GenLearnOpenGLCube();
    Mesh m_QuadMesh = GenerateQuad();

    // scene state
    Camera m_Camera = CreateCamera();
    Vec3 m_CameraEulers = {};
    Transform m_QuadTransform;
    Transform m_LightTransform;
    bool m_IsNormalMappingEnabled = true;
    bool m_IsMouseCaptured = false;
};


// public API

CStringView osc::LOGLNormalMappingTab::id()
{
    return c_TabStringID;
}

osc::LOGLNormalMappingTab::LOGLNormalMappingTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLNormalMappingTab::LOGLNormalMappingTab(LOGLNormalMappingTab&&) noexcept = default;
osc::LOGLNormalMappingTab& osc::LOGLNormalMappingTab::operator=(LOGLNormalMappingTab&&) noexcept = default;
osc::LOGLNormalMappingTab::~LOGLNormalMappingTab() noexcept = default;

osc::UID osc::LOGLNormalMappingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLNormalMappingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLNormalMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLNormalMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLNormalMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLNormalMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLNormalMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
