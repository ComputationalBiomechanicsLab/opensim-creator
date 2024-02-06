#include "LOGLNormalMappingTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>
#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <imgui.h>
#include <oscar/oscar.hpp>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/NormalMapping";

    // matches the quad used in LearnOpenGL's normal mapping tutorial
    Mesh GenerateQuad()
    {
        constexpr auto verts = std::to_array<Vec3>({
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        });
        constexpr auto normals = std::to_array<Vec3>({
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        });
        constexpr auto texCoords = std::to_array<Vec2>({
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        });
        constexpr auto indices = std::to_array<uint16_t>({
            0, 1, 2,
            0, 2, 3,
        });
        auto const tangents = CalcTangentVectors(
            MeshTopology::Triangles,
            verts,
            normals,
            texCoords,
            indices
        );

        Mesh rv;
        rv.setVerts(verts);
        rv.setNormals(normals);
        rv.setTexCoords(texCoords);
        rv.setTangents(tangents);
        rv.setIndices(indices);
        return rv;
    }

    MouseCapturingCamera CreateCamera()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        return rv;
    }

    Material CreateNormalMappingMaterial()
    {
        Texture2D diffuseMap = LoadTexture2DFromImage(
            App::load_resource("oscar_learnopengl/textures/brickwall.jpg"),
            ColorSpace::sRGB
        );
        Texture2D normalMap = LoadTexture2DFromImage(
            App::load_resource("oscar_learnopengl/textures/brickwall_normal.jpg"),
            ColorSpace::Linear
        );

        Material rv{Shader{
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedLighting/NormalMapping.frag"),
        }};
        rv.setTexture("uDiffuseMap", diffuseMap);
        rv.setTexture("uNormalMap", normalMap);

        return rv;
    }

    Material CreateLightCubeMaterial()
    {
        return Material{Shader{
            App::slurp("oscar_learnopengl/shaders/LightCube.vert"),
            App::slurp("oscar_learnopengl/shaders/LightCube.frag"),
        }};
    }
}

class osc::LOGLNormalMappingTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnMount() final
    {
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.onEvent(e);
    }

    void implOnTick() final
    {
        // rotate the quad over time
        AppClock::duration const dt = App::get().getFrameDeltaSinceAppStartup();
        auto const angle = Degrees{-10.0 * dt.count()};
        auto const axis = UnitVec3{1.0f, 0.0f, 1.0f};
        m_QuadTransform.rotation = AngleAxis(angle, axis);
    }

    void implOnDraw() final
    {
        m_Camera.onDraw();

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

        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        ImGui::Begin("controls");
        ImGui::Checkbox("normal mapping", &m_IsNormalMappingEnabled);
        ImGui::End();
    }

    // rendering state
    Material m_NormalMappingMaterial = CreateNormalMappingMaterial();
    Material m_LightCubeMaterial = CreateLightCubeMaterial();
    Mesh m_CubeMesh = GenerateLearnOpenGLCubeMesh();
    Mesh m_QuadMesh = GenerateQuad();

    // scene state
    MouseCapturingCamera m_Camera = CreateCamera();
    Transform m_QuadTransform;
    Transform m_LightTransform = {
        .scale = Vec3{0.2f},
        .position = {0.5f, 1.0f, 0.3f},
    };
    bool m_IsNormalMappingEnabled = true;
};


// public API

CStringView osc::LOGLNormalMappingTab::id()
{
    return c_TabStringID;
}

osc::LOGLNormalMappingTab::LOGLNormalMappingTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLNormalMappingTab::LOGLNormalMappingTab(LOGLNormalMappingTab&&) noexcept = default;
osc::LOGLNormalMappingTab& osc::LOGLNormalMappingTab::operator=(LOGLNormalMappingTab&&) noexcept = default;
osc::LOGLNormalMappingTab::~LOGLNormalMappingTab() noexcept = default;

UID osc::LOGLNormalMappingTab::implGetID() const
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
