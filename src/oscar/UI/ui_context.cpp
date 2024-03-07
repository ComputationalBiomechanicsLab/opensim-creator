#include "ui_context.h"

#include <IconsFontAwesome5.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/imgui_impl_sdl2.h>
#include <oscar/UI/ui_graphics_backend.h>
#include <oscar/Utils/Perf.h>
#include <SDL_events.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <string>

using namespace osc;
namespace ranges = std::ranges;

namespace
{
    constexpr auto c_IconRanges = std::to_array<ImWchar>({ ICON_MIN_FA, ICON_MAX_FA, 0 });

    // this is necessary because ImGui will take ownership, but will free the
    // font atlas with `free`, rather than `delete`, which memory sanitizers
    // like libASAN dislike (`malloc`/`free`, or `new`/`delete` - no mixes)
    template<ranges::contiguous_range Container>
    typename Container::value_type* ToMalloced(Container const& c)
    {
        using value_type = typename Container::value_type;
        using std::size;
        using std::begin;
        using std::end;

        auto* ptr = cpp20::bit_cast<value_type*>(malloc(size(c) * sizeof(value_type)));  // NOLINT(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc,hicpp-no-malloc)
        std::copy(begin(c), end(c), ptr);
        return ptr;
    }

    void AddResourceAsFont(
        ImFontConfig const& config,
        ImFontAtlas& atlas,
        ResourcePath const& path,
        ImWchar const* glyphRanges = nullptr)
    {
        std::string baseFontData = App::slurp(path);
        atlas.AddFontFromMemoryTTF(
            ToMalloced(baseFontData),  // ImGui takes ownership
            static_cast<int>(baseFontData.size()) + 1,  // +1 for NUL
            config.SizePixels,
            &config,
            glyphRanges
        );
    }
}

void osc::ui::context::Init()
{
    // init ImGui top-level context
    ImGui::CreateContext();

    ImGuiIO& io = ui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // make it so that windows can only ever be moved from the title bar
    ui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

    // load application-level ImGui config, then the user one,
    // so that the user config takes precedence
    {
        std::string const defaultINIData = App::slurp("imgui_base_config.ini");
        ImGui::LoadIniSettingsFromMemory(defaultINIData.data(), defaultINIData.size());

        // CARE: the reason this filepath is `static` is because ImGui requires that
        // the string outlives the ImGui context
        static std::string const s_UserImguiIniFilePath = (App::get().getUserDataDirPath() / "imgui.ini").string();

        ImGui::LoadIniSettingsFromDisk(s_UserImguiIniFilePath.c_str());
        io.IniFilename = s_UserImguiIniFilePath.c_str();
    }

    float dpiScaleFactor = [&]()
    {
        float dpi{};
        float hdpi{};
        float vdpi{};

        // if the user explicitly enabled high_dpi_mode...
        if (auto v = App::config().getValue("experimental_feature_flags/high_dpi_mode"); v && v->toBool()) {
            // and SDL is able to get the DPI of the given window...
            if (SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(App::upd().updUndleryingWindow()), &dpi, &hdpi, &vdpi) == 0) {
                return dpi / 96.0f;  // then calculate the scaling factor
            }
        }
        return 1.0f;  // else: assume it's an unscaled 96dpi screen
    }();

    ImFontConfig baseConfig;
    baseConfig.SizePixels = dpiScaleFactor*15.0f;
    baseConfig.PixelSnapH = true;
    baseConfig.OversampleH = 2;
    baseConfig.OversampleV = 2;
    baseConfig.FontDataOwnedByAtlas = true;
    AddResourceAsFont(baseConfig, *io.Fonts, "oscar/fonts/Ruda-Bold.ttf");

    // add FontAwesome icon support
    {
        ImFontConfig config = baseConfig;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = floor(1.5f * config.SizePixels);
        AddResourceAsFont(config, *io.Fonts, "oscar/fonts/fa-solid-900.ttf", c_IconRanges.data());
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

    ImGuiIO const& io  = ui::GetIO();

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
