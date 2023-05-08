#include "LOGLCubemapsTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/ColorSpace.hpp"
#include "src/Graphics/Cubemap.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Image.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"

#include <glm/vec3.hpp>
#include <imgui.h>
#include <SDL_events.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Cubemaps";

    osc::Cubemap LoadCubemap(std::filesystem::path const& resourcesDir)
    {
        auto textures = osc::MakeSizedArray<osc::CStringView, static_cast<size_t>(osc::CubemapFace::TOTAL)>(
            "skybox_right.jpg",
            "skybox_left.jpg",
            "skybox_top.jpg",
            "skybox_bottom.jpg",
            "skybox_front.jpg",
            "skybox_back.jpg"
        );
        static_assert(textures.size() > 1);

        // load the first face, so we know the width
        osc::Image image = osc::LoadImageFromFile(
            resourcesDir / "textures" / std::string_view{textures.front()},
            osc::ColorSpace::sRGB
        );
        OSC_THROWING_ASSERT(image.getDimensions().x == image.getDimensions().y);
        OSC_THROWING_ASSERT(image.getNumChannels() == 3);
        int32_t const width = image.getDimensions().x;

        // load all face data into the cubemap
        static_assert(static_cast<int32_t>(osc::CubemapFace::PositiveX) == 0);
        static_assert(static_cast<size_t>(osc::CubemapFace::TOTAL) == textures.size());
        osc::Cubemap cubemap{width, osc::TextureFormat::RGB24};
        cubemap.setPixelData(osc::CubemapFace::PositiveX, image.getPixelData());
        for (int32_t i = 1; i < static_cast<int32_t>(osc::CubemapFace::TOTAL); ++i)
        {
            image = osc::LoadImageFromFile(
                resourcesDir / "textures" / std::string_view{textures[i]},
                osc::ColorSpace::sRGB
            );
            OSC_THROWING_ASSERT(image.getDimensions().x == width);
            OSC_THROWING_ASSERT(image.getDimensions().y == width);
            cubemap.setPixelData(static_cast<osc::CubemapFace>(i), image.getPixelData());
        }

        return cubemap;
    }
}

class osc::LOGLCubemapsTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
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

        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopPolling();
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
        App::upd().makeMainEventLoopWaiting();
    }

    bool onEvent(SDL_Event const& e)
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

    void onDraw()
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
        m_Camera.setViewMatrixOverride(glm::mat4{glm::mat3{m_Camera.getViewMatrix()}});
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

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    struct CubeMaterial final {
        CStringView label;
        Material material;
    };
    std::array<CubeMaterial, 3> m_CubeMaterials =
    {
        CubeMaterial
        {
            "Basic",
            Material
            {
                Shader
                {
                    App::slurp("shaders/ExperimentCubemap.vert"),
                    App::slurp("shaders/ExperimentCubemap.frag"),
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
                    App::slurp("shaders/ExperimentCubemapReflection.vert"),
                    App::slurp("shaders/ExperimentCubemapReflection.frag"),
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
                    App::slurp("shaders/ExperimentCubemapRefraction.vert"),
                    App::slurp("shaders/ExperimentCubemapRefraction.frag"),
                },
            },
        },
    };
    size_t m_CubeMaterialIndex = 0;
    MaterialPropertyBlock m_CubeProperties;
    Mesh m_Cube = GenLearnOpenGLCube();
    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        App::resource("textures/container.jpg"),
        ColorSpace::sRGB
    );
    float m_IOR = 1.52f;

    Material m_SkyboxMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentCubemapSkybox.vert"),
            App::slurp("shaders/ExperimentCubemapSkybox.frag"),
        },
    };
    Mesh m_Skybox = GenCube();
    Cubemap m_Cubemap = LoadCubemap(App::get().getConfig().getResourceDir());

    Camera m_Camera;
    bool m_IsMouseCaptured = true;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
};


// public API (PIMPL)

osc::CStringView osc::LOGLCubemapsTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLCubemapsTab::LOGLCubemapsTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::LOGLCubemapsTab::LOGLCubemapsTab(LOGLCubemapsTab&&) noexcept = default;
osc::LOGLCubemapsTab& osc::LOGLCubemapsTab::operator=(LOGLCubemapsTab&&) noexcept = default;
osc::LOGLCubemapsTab::~LOGLCubemapsTab() noexcept = default;

osc::UID osc::LOGLCubemapsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLCubemapsTab::implGetName() const
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
