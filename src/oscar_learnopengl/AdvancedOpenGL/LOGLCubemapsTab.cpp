#include "LOGLCubemapsTab.hpp"

#include <oscar_learnopengl/LearnOpenGLHelpers.hpp>

#include <imgui.h>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Cubemap.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Mat3.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <SDL_events.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

using osc::App;
using osc::Camera;
using osc::ColorSpace;
using osc::CStringView;
using osc::Cubemap;
using osc::CubemapFace;
using osc::Mat3;
using osc::Mat4;
using osc::Material;
using osc::Shader;
using osc::Texture2D;
using osc::Vec2i;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/Cubemaps";
    constexpr auto c_SkyboxTextureFilenames = std::to_array<CStringView>(
    {
        "skybox_right.jpg",
        "skybox_left.jpg",
        "skybox_top.jpg",
        "skybox_bottom.jpg",
        "skybox_front.jpg",
        "skybox_back.jpg",
    });
    static_assert(c_SkyboxTextureFilenames.size() == osc::NumOptions<CubemapFace>());
    static_assert(c_SkyboxTextureFilenames.size() > 1);

    Cubemap LoadCubemap(std::filesystem::path const& resourcesDir)
    {
        // load the first face, so we know the width
        Texture2D t = osc::LoadTexture2DFromImage(
            resourcesDir / "oscar_learnopengl" / "textures" / std::string_view{c_SkyboxTextureFilenames.front()},
            ColorSpace::sRGB
        );

        Vec2i const dims = t.getDimensions();
        OSC_ASSERT(dims.x == dims.y);

        // load all face data into the cubemap
        static_assert(osc::NumOptions<CubemapFace>() == c_SkyboxTextureFilenames.size());

        Cubemap cubemap{dims.x, t.getTextureFormat()};
        cubemap.setPixelData(osc::FirstCubemapFace(), t.getPixelData());
        for (CubemapFace f = osc::Next(osc::FirstCubemapFace()); f <= osc::LastCubemapFace(); f = osc::Next(f))
        {
            t = osc::LoadTexture2DFromImage(
                resourcesDir / "oscar_learnopengl" / "textures" / std::string_view{c_SkyboxTextureFilenames[osc::ToIndex(f)]},
                ColorSpace::sRGB
            );
            OSC_ASSERT(t.getDimensions().x == dims.x);
            OSC_ASSERT(t.getDimensions().y == dims.x);
            OSC_ASSERT(t.getTextureFormat() == cubemap.getTextureFormat());
            cubemap.setPixelData(f, t.getPixelData());
        }

        return cubemap;
    }

    Camera CreateCameraThatMatchesLearnOpenGL()
    {
        Camera rv;
        rv.setPosition({0.0f, 0.0f, 3.0f});
        rv.setCameraFOV(osc::Deg2Rad(45.0f));
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
        return std::to_array(
        {
            CubeMaterial
            {
                "Basic",
                Material
                {
                    Shader
                    {
                        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.vert"),
                        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.frag"),
                    },
                },
            },
            CubeMaterial
            {
                "Reflection",
                Material
                {
                    Shader
                    {
                        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.vert"),
                        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.frag"),
                    },
                },
            },
            CubeMaterial
            {
                "Refraction",
                Material
                {
                    Shader
                    {
                        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.vert"),
                        App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.frag"),
                    },
                },
            },
        });
    }
}

class osc::LOGLCubemapsTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
        for (CubeMaterial& cubeMat : m_CubeMaterials)
        {
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
        m_IsMouseCaptured = true;
    }

    void implOnUnmount() final
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool implOnEvent(SDL_Event const& e) final
    {
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

    void implOnDraw() final
    {
        // handle mouse capturing
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
        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

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
            Transform{},
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
            Transform{},
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
        if (ImGui::BeginCombo("Cube Texturing", m_CubeMaterials.at(m_CubeMaterialIndex).label.c_str()))
        {
            for (size_t i = 0; i < m_CubeMaterials.size(); ++i)
            {
                bool selected = i == m_CubeMaterialIndex;
                if (ImGui::Selectable(m_CubeMaterials[i].label.c_str(), &selected))
                {
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
    Mesh m_Cube = GenLearnOpenGLCube();
    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        App::resource("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    float m_IOR = 1.52f;

    Material m_SkyboxMaterial
    {
        Shader
        {
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.vert"),
            App::slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.frag"),
        },
    };
    Mesh m_Skybox = GenCube();
    Cubemap m_Cubemap = LoadCubemap(App::get().getConfig().getResourceDir());

    Camera m_Camera = CreateCameraThatMatchesLearnOpenGL();
    bool m_IsMouseCaptured = true;
    Vec3 m_CameraEulers = {};
};


// public API

CStringView osc::LOGLCubemapsTab::id()
{
    return c_TabStringID;
}

osc::LOGLCubemapsTab::LOGLCubemapsTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLCubemapsTab::LOGLCubemapsTab(LOGLCubemapsTab&&) noexcept = default;
osc::LOGLCubemapsTab& osc::LOGLCubemapsTab::operator=(LOGLCubemapsTab&&) noexcept = default;
osc::LOGLCubemapsTab::~LOGLCubemapsTab() noexcept = default;

osc::UID osc::LOGLCubemapsTab::implGetID() const
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
