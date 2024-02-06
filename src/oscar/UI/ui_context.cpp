#include "ui_context.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <ImGuizmo.h>  // care: must come after imgui.h
#include <implot.h>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/ui_graphics_backend.hpp>
#include <oscar/Utils/Perf.hpp>
#include <SDL_events.h>

#include <array>
#include <filesystem>
#include <string>

namespace
{
    constexpr auto c_IconRanges = std::to_array<ImWchar>({ ICON_MIN_FA, ICON_MAX_FA, 0 });
}

void osc::ui::context::Init()
{
    // init ImGui top-level context
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // make it so that windows can only ever be moved from the title bar
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

    // load application-level ImGui config, then the user one,
    // so that the user config takes precedence
    {
        std::string defaultIni = App::resource("imgui_base_config.ini").string();
        ImGui::LoadIniSettingsFromDisk(defaultIni.c_str());

        // CARE: the reason this filepath is `static` is because ImGui requires that
        // the string outlives the ImGui context
        static std::string const s_UserImguiIniFilePath = (App::get().getUserDataDirPath() / "imgui.ini").string();

        ImGui::LoadIniSettingsFromDisk(s_UserImguiIniFilePath.c_str());
        io.IniFilename = s_UserImguiIniFilePath.c_str();
    }

    ImFontConfig baseConfig;
    baseConfig.SizePixels = 15.0f;
    baseConfig.PixelSnapH = true;
    baseConfig.OversampleH = 2;
    baseConfig.OversampleV = 2;
    std::string baseFontFile = App::resource("oscar/fonts/Ruda-Bold.ttf").string();
    io.Fonts->AddFontFromFileTTF(baseFontFile.c_str(), baseConfig.SizePixels, &baseConfig);

    // add FontAwesome icon support
    {
        ImFontConfig config = baseConfig;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = std::floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = std::floor(1.5f * config.SizePixels);

        std::string const fontFile = App::resource("oscar/fonts/fa-solid-900.ttf").string();
        io.Fonts->AddFontFromFileTTF(
            fontFile.c_str(),
            config.SizePixels,
            &config,
            c_IconRanges.data()
        );
    }

    // init ImGui for SDL2 /w OpenGL
    ImGui_ImplSDL2_InitForOpenGL(
        App::upd().updUndleryingWindow(),
        App::upd().updUnderlyingOpenGLContext()
    );

    // init ImGui for OpenGL
    graphics_backend::Init();

    ImGuiApplyDarkTheme();

    // init extra parts (plotting, gizmos, etc.)
    ImPlot::CreateContext();
}

void osc::ui::context::Shutdown()
{
    ImPlot::DestroyContext();

    graphics_backend::Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool osc::ui::context::OnEvent(SDL_Event const& e)
{
    ImGui_ImplSDL2_ProcessEvent(&e);

    ImGuiIO const& io  = ImGui::GetIO();

    bool handledByImgui = false;

    if (io.WantCaptureKeyboard && (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP))
    {
        handledByImgui = true;
    }

    if (io.WantCaptureMouse && (e.type == SDL_MOUSEWHEEL || e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN))
    {
        handledByImgui = true;
    }

    return handledByImgui;
}

void osc::ui::context::NewFrame()
{
    graphics_backend::NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // extra parts
    ImGuizmo::BeginFrame();
}

void osc::ui::context::Render()
{
    {
        OSC_PERF("ImGuiRender/Render");
        ImGui::Render();
    }

    {
        OSC_PERF("ImGuiRender/ImGui_ImplOscarGfx_RenderDrawData");
        graphics_backend::RenderDrawData(ImGui::GetDrawData());
    }
}
