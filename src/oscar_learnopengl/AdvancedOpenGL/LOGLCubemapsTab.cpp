#include "LOGLCubemapsTab.h"

#include <oscar_learnopengl/LearnOpenGLHelpers.h>
#include <oscar_learnopengl/MouseCapturingCamera.h>

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <memory>
#include <optional>
#include <string_view>

using namespace osc::literals;
using namespace osc;

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

    Cubemap LoadCubemap(ResourceLoader& rl)
    {
        // load the first face, so we know the width
        Texture2D t = LoadTexture2DFromImage(
            rl.open(ResourcePath{"oscar_learnopengl/textures"} / std::string_view{c_SkyboxTextureFilenames.front()}),
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
                rl.open(ResourcePath{"oscar_learnopengl/textures"} / std::string_view{c_SkyboxTextureFilenames[ToIndex(f)]}),
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
        rv.setVerticalFOV(45_deg);
        rv.setNearClippingPlane(0.1f);
        rv.setFarClippingPlane(100.0f);
        rv.setBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
        return rv;
    }

    struct CubeMaterial final {
        CStringView label;
        Material material;
    };

    std::array<CubeMaterial, 3> CreateCubeMaterials(ResourceLoader& rl)
    {
        return std::to_array({
            CubeMaterial{
                "Basic",
                Material{Shader{
                    rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.vert"),
                    rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Basic.frag"),
                }},
            },
            CubeMaterial{
                "Reflection",
                Material{Shader{
                    rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.vert"),
                    rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Reflection.frag"),
                }},
            },
            CubeMaterial{
                "Refraction",
                Material{Shader{
                    rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.vert"),
                    rl.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Refraction.frag"),
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
        m_Camera.setPixelRect(ui::GetMainViewportWorkspaceScreenRect());

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
            identity<Transform>(),
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
            identity<Transform>(),
            m_SkyboxMaterial,
            m_Camera
        );
        m_Camera.renderToScreen();
        m_Camera.setViewMatrixOverride(std::nullopt);
        m_Camera.setClearFlags(CameraClearFlags::Default);
    }

    void draw2DUI()
    {
        ui::Begin("controls");
        if (ui::BeginCombo("Cube Texturing", m_CubeMaterials.at(m_CubeMaterialIndex).label)) {
            for (size_t i = 0; i < m_CubeMaterials.size(); ++i) {
                bool selected = i == m_CubeMaterialIndex;
                if (ui::Selectable(m_CubeMaterials[i].label, &selected)) {
                    m_CubeMaterialIndex = i;
                }
            }
            ui::EndCombo();
        }
        ui::InputFloat("IOR", &m_IOR);
        ui::End();
    }

    ResourceLoader m_Loader = App::resource_loader();

    std::array<CubeMaterial, 3> m_CubeMaterials = CreateCubeMaterials(m_Loader);
    size_t m_CubeMaterialIndex = 0;
    MaterialPropertyBlock m_CubeProperties;
    Mesh m_Cube = GenerateLearnOpenGLCubeMesh();
    Texture2D m_ContainerTexture = LoadTexture2DFromImage(
        m_Loader.open("oscar_learnopengl/textures/container.jpg"),
        ColorSpace::sRGB
    );
    float m_IOR = 1.52f;

    Material m_SkyboxMaterial{Shader{
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.vert"),
        m_Loader.slurp("oscar_learnopengl/shaders/AdvancedOpenGL/Cubemaps/Skybox.frag"),
    }};
    Mesh m_Skybox = GenerateCubeMesh();
    Cubemap m_Cubemap = LoadCubemap(m_Loader);

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
