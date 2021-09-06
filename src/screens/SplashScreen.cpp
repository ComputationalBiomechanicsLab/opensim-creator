#include "SplashScreen.hpp"

#include "src/3d/shaders/PlainTextureShader.hpp"
#include "src/3d/Constants.hpp"
#include "src/3d/Gl.hpp"
#include "src/3d/GlGlm.hpp"
#include "src/3d/InstancedRenderer.hpp"
#include "src/3d/Model.hpp"
#include "src/3d/Texturing.hpp"
#include "src/screens/LoadingScreen.hpp"
#include "src/ui/main_menu.hpp"
#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/Styling.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <memory>

using namespace osc;

static gl::Texture2D loadImageResourceIntoTexture(char const* res_pth) {
    return loadImageAsTexture(App::resource(res_pth).string().c_str(), TexFlag_FlipPixelsVertically).texture;
}

struct SplashScreen::Impl final {
    // used to render quads (e.g. logo banners)
    PlainTextureShader pts;

    // TODO: fix this shit packing
    gl::ArrayBuffer<glm::vec3> quadVBO{GenTexturedQuad().verts};
    gl::ArrayBuffer<glm::vec2> quadStandardUVs{GenTexturedQuad().texcoords};
    gl::VertexArray quadPtsVAO = [this]() {
        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quadVBO);
        gl::VertexAttribPointer(pts.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(pts.aPos);
        gl::BindBuffer(quadStandardUVs);
        gl::VertexAttribPointer(pts.aTexCoord, false, sizeof(glm::vec2), 0);
        gl::EnableVertexAttribArray(pts.aTexCoord);
        gl::BindVertexArray();
        return rv;
    }();

    // main menu (top bar) states
    ui::main_menu::file_tab::State mmState;

    // main app logo, blitted to top of the screen
    gl::Texture2D logo = loadImageResourceIntoTexture("logo.png");

    // CZI attributation logo, blitted to bottom of screen
    gl::Texture2D czLogo = loadImageResourceIntoTexture("chanzuckerberg_logo.png");

    // TUD attributation logo, blitted to bottom of screen
    gl::Texture2D tudLogo = loadImageResourceIntoTexture("tud_logo.png");

    // 3D stuff: for the background image
    InstancedRenderer renderer;
    InstancedDrawlist drawlist = []() {
        glm::mat4x3 mmtx = []() {
            glm::mat4 rv = glm::identity<glm::mat4>();

            // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
            // floor down *slightly* to prevent Z fighting from planes rendered from the
            // model itself (the contact planes, etc.)
            rv = glm::translate(rv, {0.0f, -0.005f, 0.0f});
            rv = glm::rotate(rv, fpi2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {100.0f, 100.0f, 1.0f});

            return rv;
        }();
        glm::mat3 normalMtx = NormalMatrix(mmtx);
        Rgba32 color = Rgba32FromU32(0xffffffff);
        Mesh quadData = GenTexturedQuad();
        for (auto& uv : quadData.texcoords) {
            uv *= 200.0f;
        }
        InstanceableMeshdata md = uploadMeshdataForInstancing(quadData);
        std::shared_ptr<gl::Texture2D> tex = std::make_shared<gl::Texture2D>(genChequeredFloorTexture());

        DrawlistCompilerInput inp;
        inp.ninstances = 1;
        inp.modelMtxs = &mmtx;
        inp.normalMtxs = &normalMtx;
        inp.colors = &color;
        inp.meshes = &md;
        inp.textures = &tex;
        inp.rimIntensities = nullptr;

        InstancedDrawlist rv;
        uploadInputsToDrawlist(inp, rv);
        return rv;
    }();
    InstancedRendererParams params;
    PolarPerspectiveCamera camera;

    // top-level UI state that's shared between screens
    std::shared_ptr<MainEditorState> mes;

    Impl(std::shared_ptr<MainEditorState> mes_) : mes{std::move(mes_)} {
        log::info("splash screen constructed");
        camera.phi = fpi4;
        camera.radius = 10.0f;
    }

    void drawQuad(glm::mat4 const& mvp, gl::Texture2D& tex) {
        gl::UseProgram(pts.program);
        gl::Uniform(pts.uMVP, mvp);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(tex);
        gl::Uniform(pts.uSampler0, gl::textureIndex<GL_TEXTURE0>());
        gl::BindVertexArray(quadPtsVAO);
        gl::DrawArrays(GL_TRIANGLES, 0, quadVBO.sizei());
        gl::BindVertexArray();
    }
};


// public API

osc::SplashScreen::SplashScreen() :
    m_Impl{new Impl{std::make_shared<MainEditorState>()}} {
}

osc::SplashScreen::SplashScreen(std::shared_ptr<MainEditorState> mes_) :
    m_Impl{new Impl{std::move(mes_)}} {
}

osc::SplashScreen::~SplashScreen() noexcept = default;

void osc::SplashScreen::onMount() {
    osc::ImGuiInit();
}

void osc::SplashScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::SplashScreen::onEvent(SDL_Event const& e) {
    osc::ImGuiOnEvent(e);
}

void osc::SplashScreen::tick(float dt) {
    m_Impl->camera.theta += dt * 0.015f;
}

void osc::SplashScreen::draw() {
    osc::ImGuiNewFrame();

    Impl& impl = *m_Impl;

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    constexpr glm::vec2 menuDims = {700.0f, 500.0f};

    App& app = App::cur();
    glm::ivec2 windowDimsInt = app.idims();
    glm::vec2 windowDims{windowDimsInt};

    gl::Enable(GL_BLEND);

    // draw chequered floor background
    {
        impl.renderer.setDims(app.idims());
        impl.renderer.setMsxaaSamples(app.getSamples());
        impl.params.viewMtx = impl.camera.getViewMtx();
        impl.params.projMtx = impl.camera.getProjMtx(impl.renderer.getAspectRatio());
        impl.params.viewPos = impl.camera.getPos();

        impl.renderer.render(impl.params, impl.drawlist);
        impl.drawQuad(glm::mat4{1.0f}, impl.renderer.getOutputTexture());
    }

    // draw logo just above the menu
    {
        glm::vec2 desiredLogoDims = {128.0f, 128.0f};
        glm::vec2 scale = desiredLogoDims / windowDims;
        float yAboveMenu = (menuDims.y + desiredLogoDims.y + 64.0f) / windowDims.y;

        glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {0.0f, yAboveMenu, 0.0f});
        glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

        glm::mat4 model = translate_xform * scale_xform;

        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        impl.drawQuad(model, impl.logo);
    }

    if (ImGui::BeginMainMenuBar()) {
        ui::main_menu::file_tab::draw(impl.mmState, impl.mes);
        ui::main_menu::about_tab::draw();
        ImGui::EndMainMenuBar();
    }

    // center the menu
    {
        glm::vec2 menuPos = (windowDims - menuDims) / 2.0f;
        ImGui::SetNextWindowPos(menuPos);
        ImGui::SetNextWindowSize(ImVec2(menuDims.x, -1));
        ImGui::SetNextWindowSizeConstraints(menuDims, menuDims);
    }

    bool b = true;
    if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {

        // `new` button
        {
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
            if (ImGui::Button(ICON_FA_FILE_ALT " New Model (Ctrl+N)")) {
                ui::main_menu::action_new_model(impl.mes);
            }
            ImGui::PopStyleColor(2);
        }

        ImGui::SameLine();

        // `open` button
        {
            if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Model (Ctrl+O)")) {
                ui::main_menu::action_open_model(impl.mes);
            }
        }

        ImGui::Dummy(ImVec2{0.0f, 10.0f});

        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int id = 0;

        ImGui::Columns(2);

        // left column: recent files
        ImGui::TextUnformatted("Recent files:");
        ImGui::Dummy(ImVec2{0.0f, 3.0f});

        if (!impl.mmState.recent_files.empty()) {
            // iterate in reverse: recent files are stored oldest --> newest
            for (auto it = impl.mmState.recent_files.rbegin(); it != impl.mmState.recent_files.rend(); ++it) {
                RecentFile const& rf = *it;
                ImGui::PushID(++id);
                if (ImGui::Button(rf.path.filename().string().c_str())) {
                    app.requestTransition<osc::LoadingScreen>(impl.mes, rf.path);
                }
                ImGui::PopID();
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
            ImGui::TextWrapped("No files opened recently. Try:");
            ImGui::BulletText("Creating a new model (Ctrl+N)");
            ImGui::BulletText("Opening an existing model (Ctrl+O)");
            ImGui::BulletText("Opening an example (right-side)");
            ImGui::PopStyleColor();
        }
        ImGui::NextColumn();

        // right column: example model files
        if (!impl.mmState.example_osims.empty()) {
            ImGui::TextUnformatted("Example files:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            for (std::filesystem::path const& ex : impl.mmState.example_osims) {
                ImGui::PushID(++id);
                if (ImGui::Button(ex.filename().string().c_str())) {
                    app.requestTransition<LoadingScreen>(impl.mes, ex);
                }
                ImGui::PopID();
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }
    ImGui::End();

    // draw TUD logo below menu, slightly to the left
    {
        glm::vec2 desiredLogoDims = {128.0f, 128.0f};
        glm::vec2 scale = desiredLogoDims / windowDims;
        float xLeftByLogoWidth = -desiredLogoDims.x / windowDims.x;
        float yBelowMenu = -(25.0f + menuDims.y + desiredLogoDims.y) / windowDims.y;

        glm::mat4 translateMtx = glm::translate(glm::mat4{1.0f}, {xLeftByLogoWidth, yBelowMenu, 0.0f});
        glm::mat4 scaleMtx = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

        glm::mat4 model = translateMtx * scaleMtx;

        gl::Enable(GL_BLEND);
        gl::Disable(GL_DEPTH_TEST);
        impl.drawQuad(model, impl.tudLogo);
    }

    // draw CZI logo below menu, slightly to the right
    {
        glm::vec2 desiredLogoDims = {128.0f, 128.0f};
        glm::vec2 scale = desiredLogoDims / windowDims;
        float xRightByLogoWidth = desiredLogoDims.x / windowDims.x;
        float yBelowMenu = -(25.0f + menuDims.y + desiredLogoDims.y) / windowDims.y;

        glm::mat4 translateMtx = glm::translate(glm::mat4{1.0f}, {xRightByLogoWidth, yBelowMenu, 0.0f});
        glm::mat4 scaleMtx = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

        glm::mat4 model = translateMtx * scaleMtx;

        gl::Disable(GL_DEPTH_TEST);
        gl::Enable(GL_BLEND);
        impl.drawQuad(model, impl.czLogo);
    }

    osc::ImGuiRender();
}
