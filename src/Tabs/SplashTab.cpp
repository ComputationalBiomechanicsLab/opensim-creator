#include "SplashTab.hpp"

#include "osc_config.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/MultisampledRenderBuffers.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/LoadingTab.hpp"
#include "src/Tabs/MeshImporterTab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <filesystem>
#include <memory>


static gl::Texture2D LoadImageResourceIntoTexture(char const* res_pth)
{
	return osc::loadImageAsTexture(osc::App::resource(res_pth).string().c_str(), osc::TexFlag_FlipPixelsVertically).texture;
}

static glm::mat4x3 GenerateFloorModelMatrix()
{
	// rotate from XY (+Z dir) to ZY (+Y dir)
	glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -osc::fpi2, {1.0f, 0.0f, 0.0f});

	// make floor extend far in all directions
	rv = glm::scale(glm::mat4{1.0f}, {100.0f, 1.0f, 100.0f}) * rv;

	// lower slightly, so that it doesn't conflict with OpenSim model planes
	// that happen to lie at Z==0
	rv = glm::translate(glm::mat4{1.0f}, {0.0f, -0.0001f, 0.0f}) * rv;

	return glm::mat4x3{rv};
}

namespace
{
    struct SizedTexture {
        gl::Texture2D texture;
        glm::vec2 dimensions;

        SizedTexture(gl::Texture2D texture_, glm::vec2 dimensions_) :
            texture{std::move(texture_)},
            dimensions{std::move(dimensions_)}
        {
        }
    };
}

static std::unique_ptr<SizedTexture> GenerateBackgroundImage(glm::vec2 dimensions)
{
    glm::vec3 lightDir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 backgroundCol = {0.89f, 0.89f, 0.89f, 1.0f};
    osc::PolarPerspectiveCamera camera;
    camera.phi = osc::fpi4/1.5f;
    camera.radius = 10.0f;
    camera.theta = osc::fpi4;
    glm::mat4 floorMat = GenerateFloorModelMatrix();
    glm::mat4 floorNormalMat = osc::ToNormalMatrix(floorMat);
    gl::Texture2D chequer = osc::genChequeredFloorTexture();
    std::shared_ptr<osc::Mesh> floorMesh = osc::App::meshes().getFloorMesh();

    // create render buffers
    osc::MultisampledRenderBuffers bufs{dimensions, osc::App::get().getMSXAASamplesRecommended()};

    // setup top-level drawcall state
    gl::Viewport(0, 0, bufs.getWidth(), bufs.getHeight());
    gl::Enable(GL_DEPTH_TEST);
    gl::BindFramebuffer(GL_FRAMEBUFFER, bufs.updRenderingFBO());
    gl::ClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set up shader state
    osc::GouraudShader& s = osc::App::shader<osc::GouraudShader>();
    gl::UseProgram(s.program);
    gl::Uniform(s.uProjMat, camera.getProjMtx(dimensions.x/dimensions.y));
    gl::Uniform(s.uViewMat, camera.getViewMtx());
    gl::Uniform(s.uModelMat, floorMat);
    gl::Uniform(s.uNormalMat, floorNormalMat);
    gl::Uniform(s.uDiffuseColor, {1.0f, 1.0f, 1.0f, 1.0f});
    gl::Uniform(s.uLightDir, lightDir);
    gl::Uniform(s.uLightColor, lightCol);
    gl::Uniform(s.uViewPos, camera.getPos());
    gl::Uniform(s.uIsTextured, true);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(chequer);
    gl::Uniform(s.uSampler0, gl::textureIndex<GL_TEXTURE0>());

    // draw
    gl::BindVertexArray(floorMesh->GetVertexArray());
    floorMesh->Draw();
    gl::BindVertexArray();

    // blit to not-antialiased texture
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, bufs.updRenderingFBO());
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, bufs.updSceneFBO());
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::BlitFramebuffer(0, 0, bufs.getWidth(), bufs.getHeight(), 0, 0, bufs.getWidth(), bufs.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);

    // extract texture to output
    return std::make_unique<SizedTexture>(std::move(bufs.updSceneTexture()), dimensions);    
}

class osc::SplashTab::Impl final {
public:
	Impl(MainUIStateAPI* parent) : m_Parent{std::move(parent)}
	{
	}

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

	void onMount()
	{
		App::upd().makeMainEventLoopWaiting();
	}

	void onUnmount()
	{
		App::upd().makeMainEventLoopPolling();
	}

	bool onEvent(SDL_Event const& e)
	{
        if (e.type == SDL_DROPFILE && e.drop.file != nullptr && CStrEndsWith(e.drop.file, ".osim"))
		{
            UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, e.drop.file);
            m_Parent->selectTab(tabID);
            return true;
		}
        return false;
	}

	void onTick()
	{
	}

	void drawMainMenu()
	{
        m_MainMenuFileTab.draw(m_Parent);
        m_MainMenuAboutTab.draw();
	}

	void onDraw()
	{
        drawBackground();
        drawLogo();
        drawMenu();
        drawTUDLogo();
        drawCZLogo();
        drawVersionInfo();
	}

private:
    Rect getTabScreenRect()
    {
        return osc::GetMainViewportWorkspaceScreenRect();
    }

    Rect getMainMenuRect()
    {
        constexpr glm::vec2 menuDims = {700.0f, 500.0f};

        Rect tabRect = getTabScreenRect();
        
        Rect rv{};
        rv.p1 = tabRect.p1 + (Dimensions(tabRect) - menuDims) / 2.0f;
        rv.p2 = rv.p1 + menuDims;
        return rv;
    }

    void drawBackground()
    {
        Rect screenRect = getTabScreenRect();
        ImGui::SetNextWindowPos(screenRect.p1);
        ImGui::SetNextWindowSize(Dimensions(screenRect));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ImGui::Begin("##splashscreenbackground", nullptr, GetMinimalWindowFlags());
        ImGui::PopStyleVar();

        if (!m_MaybeBackgroundImage)
        {
            m_MaybeBackgroundImage = GenerateBackgroundImage(Dimensions(screenRect));
        }
        else if (m_MaybeBackgroundImage->dimensions != Dimensions(screenRect))
        {
            m_MaybeBackgroundImage.reset();
            m_MaybeBackgroundImage = GenerateBackgroundImage(Dimensions(screenRect));
        }

        osc::DrawTextureAsImGuiImage(m_MaybeBackgroundImage->texture, m_MaybeBackgroundImage->dimensions);

        ImGui::End();
    }

    void drawLogo()
    {
        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        Rect mmr = getMainMenuRect();       

        glm::vec2 loc
        {
            mmr.p1.x + Dimensions(mmr).x/2.0f - logoDims.x/2.0f,
            mmr.p1.y - padding - logoDims.y
        };

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##osclogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_OscLogo, logoDims);
        ImGui::End();
    }

    void drawMenu()
    {
        {
            Rect mmr = getMainMenuRect();
            glm::vec2 mmrDims = Dimensions(mmr);
            ImGui::SetNextWindowPos(mmr.p1);
            ImGui::SetNextWindowSize(ImVec2(Dimensions(mmr).x, -1));
            ImGui::SetNextWindowSizeConstraints(Dimensions(mmr), Dimensions(mmr));
        }        

        if (ImGui::Begin("Splash screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            // `import meshes` button
            {
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
                if (ImGui::Button(ICON_FA_MAGIC " Import Meshes"))
                {
                    UID tabID = m_Parent->addTab<MeshImporterTab>(m_Parent);
                    m_Parent->selectTab(tabID);
                }
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();

            // `new` button
            {
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
                if (ImGui::Button(ICON_FA_FILE_ALT " New Model (Ctrl+N)"))
                {
                    actionNewModel(m_Parent);
                }
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();

            // `open` button
            if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Model (Ctrl+O)"))
            {
                actionOpenModel(m_Parent);
            }

            ImGui::SameLine();

            // `docs` button
            if (ImGui::Button(ICON_FA_BOOK " Open Documentation"))
            {
                OpenPathInOSDefaultApplication(osc::App::get().getConfig().getHTMLDocsDir() / "index.html");
            }

            ImGui::Dummy({0.0f, 10.0f});

            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int id = 0;

            ImGui::Columns(2);

            // left column: recent files
            ImGui::TextUnformatted("Recent files:");
            ImGui::Dummy({0.0f, 3.0f});

            if (!m_MainMenuFileTab.recentlyOpenedFiles.empty())
            {
                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = m_MainMenuFileTab.recentlyOpenedFiles.rbegin(); it != m_MainMenuFileTab.recentlyOpenedFiles.rend(); ++it)
                {
                    RecentFile const& rf = *it;
                    ImGui::PushID(++id);
                    if (ImGui::Button(rf.path.filename().string().c_str()))
                    {
                        UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, rf.path);
                        m_Parent->selectTab(tabID);
                    }
                    ImGui::PopID();
                }
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
                ImGui::TextWrapped("No files opened recently. Try:");
                ImGui::BulletText("Creating a new model (Ctrl+N)");
                ImGui::BulletText("Opening an existing model (Ctrl+O)");
                ImGui::BulletText("Opening an example (right-side)");
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();

            // right column: example model files
            if (!m_MainMenuFileTab.exampleOsimFiles.empty())
            {
                ImGui::TextUnformatted("Example files:");
                ImGui::Dummy({0.0f, 3.0f});

                for (std::filesystem::path const& ex : m_MainMenuFileTab.exampleOsimFiles)
                {
                    ImGui::PushID(++id);
                    if (ImGui::Button(ex.filename().string().c_str()))
                    {
                        UID tabID = m_Parent->addTab<LoadingTab>(m_Parent, ex);
                        m_Parent->selectTab(tabID);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::NextColumn();

            ImGui::Columns();
        }
        ImGui::End();
    }

    void drawTUDLogo()
    {
        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        Rect mmr = getMainMenuRect();

        glm::vec2 loc
        {
            (mmr.p1.x + mmr.p2.x)/2.0f - padding - logoDims.x,
            mmr.p2.y + padding,
        };

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##tudlogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_TudLogo, logoDims);
        ImGui::End();
    }

    void drawCZLogo()
    {
        constexpr glm::vec2 logoDims = {128.0f, 128.0f};
        constexpr float padding = 25.0f;

        Rect mmr = getMainMenuRect();

        glm::vec2 loc
        {
            (mmr.p1.x + mmr.p2.x)/2.0f + padding,
            mmr.p2.y + padding,
        };

        ImGui::SetNextWindowPos(loc);
        ImGui::Begin("##czlogo", nullptr, GetMinimalWindowFlags());
        osc::DrawTextureAsImGuiImage(m_CziLogo, logoDims);
        ImGui::End();
    }

    void drawVersionInfo()
    {
        Rect tabRect = getTabScreenRect();
        float h = ImGui::GetTextLineHeightWithSpacing();
        constexpr float padding = 5.0f;

        glm::vec2 pos{};
        pos.x = tabRect.p1.x + padding;
        pos.y = tabRect.p2.y - h - padding;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
        char const* content = "OpenSim Creator v" OSC_VERSION_STRING " (build " OSC_BUILD_ID ")";
        dl->AddText(pos, color, content);
    }

    // tab unique ID
    UID m_ID;

    // tab name
    std::string m_Name = ICON_FA_HOME;

    // tab parent
    MainUIStateAPI* m_Parent;

    // bg image of the floor
    std::unique_ptr<SizedTexture> m_MaybeBackgroundImage = nullptr;

	// main app logo, blitted to top of the screen
	gl::Texture2D m_OscLogo = LoadImageResourceIntoTexture("logo.png");

	// CZI attributation logo, blitted to bottom of screen
	gl::Texture2D m_CziLogo = LoadImageResourceIntoTexture("chanzuckerberg_logo.png");

	// TUD attributation logo, blitted to bottom of screen
	gl::Texture2D m_TudLogo = LoadImageResourceIntoTexture("tud_logo.png");

	// main menu (top bar) states
	MainMenuFileTab m_MainMenuFileTab;
	MainMenuAboutTab m_MainMenuAboutTab;

    LogViewer m_LogViewer;
};


// public API

osc::SplashTab::SplashTab(MainUIStateAPI* parent) :
	m_Impl{new Impl{std::move(parent)}}
{
}

osc::SplashTab::SplashTab(SplashTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SplashTab& osc::SplashTab::operator=(SplashTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::SplashTab::~SplashTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::SplashTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::SplashTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::SplashTab::implParent() const
{
    return m_Impl->parent();
}

void osc::SplashTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::SplashTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::SplashTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::SplashTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::SplashTab::implOnDrawMainMenu()
{
    m_Impl->drawMainMenu();
}

void osc::SplashTab::implOnDraw()
{
    m_Impl->onDraw();
}
