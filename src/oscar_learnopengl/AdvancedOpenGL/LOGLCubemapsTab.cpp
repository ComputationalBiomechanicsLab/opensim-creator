#include "LOGLCubemapsTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>
#include <oscar_learnopengl/MouseCapturingCamera.hpp>

#include <imgui.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Cubemap.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Mat3.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <SDL_events.h>

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

using namespace osc::literals;
using osc::App;
using osc::ColorSpace;
using osc::CStringView;
using osc::Cubemap;
using osc::CubemapFace;
using osc::FirstCubemapFace;
using osc::LastCubemapFace;
using osc::LoadTexture2DFromImage;
using osc::Mat3;
using osc::Mat4;
using osc::Material;
using osc::MouseCapturingCamera;
using osc::Next;
using osc::NumOptions;
using osc::Shader;
using osc::Texture2D;
using osc::ToIndex;
using osc::UID;
using osc::Vec2i;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/Cubemaps";
    constexpr auto c_SkyboxTextureFilenames = std::to_array<CStringView>({
        "skybox_right.jpg",
        "skybox_left.jpg",
        "skybox_top.jpg",
        "skybox_bottom.jpg",
        "skybox_front.jpg",
        "skybox_back.jpg",
    });
    static_assert(c_SkyboxTextureFilenames.size() == NumOptions<CubemapFace>());
    static_assert(c_SkyboxTextureFilenames.size() > 1);

    Cubemap LoadCubemap(std::filesystem::path const& resourcesDir)
    {
        // load the first face, so we know the width
        Texture2D t = LoadTexture2DFromImage(
            resourcesDir / "oscar_learnopengl" / "textures" / std::string_view{c_SkyboxTextureFilenames.front()},
            ColorSpace::sRGB
        );

        Vec2i const dims = t.getDimensions();
        OSC_ASSERT(dims.x == dims.y);

        // load all face data into the cubemap
        static_assert(NumOptions<CubemapFace>() == c_SkyboxTextureFilenames.size());

        Cubemap cubemap{dims.x, t.getTextureFormat()};
        cubemap.setPixelData(FirstCubemapFace(), t.getPixelData());
        for (CubemapFace f = Next(FirstCubemapFace()); f <= LastCubemapFace(); f = Next(f))
        {
            t = LoadTexture2DFromImage(
                resourcesDir / "oscar_learnopengl" / "textures" / std::string_view{c_SkyboxTextureFilenames[ToIndex(f)]},
                ColorSpace::sRGB
            );
            OSC_ASSERT(t.getDimensions().x == dims.x);
            OSC_ASSERT(t.getDimensions().y == dims.x);
            OSC_ASSERT(t.getTextureFormat() == cubemap.getTextureFormat());
            cubemap.setPixelData(f, t.getPixelData());
        }

        return cubemap;
    }

    MouseCapturingCamera CreateCameraThatMatchesLearnOpenGL()
    {
        MouseCapturingCamera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    struct CubeMaterial final {
        CStringView label;
        Material material;
    };

    std::array<CubeMaterial, 3> CreateCubeMaterials()
    {
        return std::to_array({
            CubeMaterial{
                "Basic",
                Material{Shader{
                    App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.vert"),
                    App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.frag"),
                }},
            },
            CubeMaterial{
                "Reflection",
                Material{Shader{
                    App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.vert"),
                    App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.frag"),
                }},
            },
            CubeMaterial{
                "Refraction",
                Material{Shader{
                    App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.vert"),
                    App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.frag"),
                }},
            },
        });
    }
}

class osc::LOGLCubemapsTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        for (CubeMaterial& cubeMat : m_CubeMaterials) {
            cubeMat.material.setTexture("uTexture", m_ContainerTexture);
            cubeMat.material.setCubemap("uSkybox", m_Cubemap);
        }

        // set the depth function to LessOrEqual because the skybox shader
        // performs a trick in which it sets gl_Position = v.xyww in order
        // to guarantee that the depth of all fragments in the skybox is
        // the highest possible depth, so that it fails an early depth
        // test if anything is drawn over it in the scene (reduces
        // fragment shader pressure)
        m_SkyboxMaterial.setCubemap("uSkybox", m_Cubemap);
        m_SkyboxMaterial.setDepthFunction(DepthFunction::LessOrEqual);
    }

private:
    void implOnMount() final
    {
        App::upd().makeMainEventLoopPolling();
        m_Camera.onMount();
    }

    void implOnUnmount() final
    {
        m_Camera.onUnmount();
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
        return m_Camera.onEvent(e);
    }

    void implOnDraw() final
    {
        m_Camera.onDraw();

        // clear screen and ensure camera has correct pixel rect
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());

        drawInSceneCube();
        drawSkybox();
        draw2DUI();
    }

    void drawInSceneCube()
    {
        m_CubeProperties.setVec3("uCameraPos", m_Camera.getPosition());
        m_CubeProperties.setFloat("uIOR", m_IOR);
        Graphics::DrawMesh(
            m_Cube,
            Identity<Transform>(),
            m_CubeMaterials.at(m_CubeMaterialIndex).material,
            m_Camera,
            m_CubeProperties
        );
        m_Camera.renderToScreen();
    }

    void drawSkybox()
    {
        m_Camera.setClearFlags(CameraClearFlags::Nothing);
        m_Camera.setViewMatrixOverride(Mat4{Mat3{m_Camera.getViewMatrix()}});
        Graphics::DrawMesh(
            m_Skybox,
            Identity<Transform>(),
            m_SkyboxMaterial,
            m_Camera
        );
        m_Camera.renderToScreen();
        m_Camera.setViewMatrixOverride(std::nullopt);
        m_Camera.setClearFlags(CameraClearFlags::Default);
    }

    void draw2DUI()
    {
        ImGui::Begin("controls");
        if (ImGui::BeginCombo("Cube Texturing", m_CubeMaterials.at(m_CubeMaterialIndex).label.c_str())) {
            for (size_t i = 0; i < m_CubeMaterials.size(); ++i) {
                bool selected = i == m_CubeMaterialIndex;
                if (ImGui::Selectable(m_CubeMaterials[i].label.c_str(), &selected)) {
                    m_CubeMaterialIndex = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::InputFloat("IOR", &m_IOR);
        ImGui::End();
    }

    std::array<CubeMaterial, 3> m_CubeMaterials = CreateCubeMaterials();
    size_t m_CubeMaterialIndex = 0;
    MaterialPropertyBlock m_CubeProperties;
    Mesh m_Cube = GenerateLearnOpenGLCubeMesh();
    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    float m_IOR = 1.52f;

    Material m_SkyboxMaterial{Shader{
        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.vert"),
        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.frag"),
    }};
    Mesh m_Skybox = GenerateCubeMesh();
    Cubemap m_Cubemap = LoadCubemap(App::get().getConfig().getResourceDir());

    MouseCapturingCamera m_Camera = CreateCameraThatMatchesLearnOpenGL();
};


// public API

CStringView osc::LOGLCubemapsTab::id()
{
    return c_TabStringID;
}

osc::LOGLCubemapsTab::LOGLCubemapsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLCubemapsTab::LOGLCubemapsTab(LOGLCubemapsTab&&) noexcept = default;
osc::LOGLCubemapsTab& osc::LOGLCubemapsTab::operator=(LOGLCubemapsTab&&) noexcept = default;
osc::LOGLCubemapsTab::~LOGLCubemapsTab() noexcept = default;

UID osc::LOGLCubemapsTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLCubemapsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLCubemapsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLCubemapsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLCubemapsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLCubemapsTab::implOnDraw()
{
    m_Impl->onDraw();
}
