#include "ui_context.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/imgui_impl_sdl2.h>
#include <oscar/UI/ui_graphics_backend.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Perf.h>

#include <imgui.h>
#include <ImGuizmo.h>
#include <implot.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <string>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
#ifndef EMSCRIPTEN
    // this is necessary because ImGui will take ownership and be responsible for
    // freeing the memory with `ImGui::MemFree`
    char* to_imgui_allocated_copy(std::span<const char> span)
    {
        auto* ptr = cpp20::bit_cast<char*>(ImGui::MemAlloc(span.size_bytes()));
        rgs::copy(span, ptr);
        return ptr;
    }

    void add_resource_as_font(
        const ImFontConfig& config,
        ImFontAtlas& atlas,
        const ResourcePath& path,
        const ImWchar* glyph_ranges = nullptr)
    {
        const std::string base_font_data = App::slurp(path);
        const std::span<const char> data_including_nul_terminator{base_font_data.data(), base_font_data.size() + 1};

        atlas.AddFontFromMemoryTTF(
            to_imgui_allocated_copy(data_including_nul_terminator),
            static_cast<int>(data_including_nul_terminator.size()),
            config.SizePixels,
            &config,
            glyph_ranges
        );
    }
#endif
}

void osc::ui::context::init()
{
    // ensure ImGui uses the same allocator as the rest of
    // our (C++ stdlib) application
    ImGui::SetAllocatorFunctions(
        [](size_t count, [[maybe_unused]] void* user_data) { return ::operator new(count); },
        [](void* ptr, [[maybe_unused]] void* user_data) { ::operator delete(ptr); }
    );

    // init ImGui top-level context
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // make it so that windows can only ever be moved from the title bar
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // load application-level ImGui settings, then the user one,
    // so that the user settings takes precedence
#ifdef EMSCRIPTEN
    io.IniFilename = nullptr;
#else
    float dpi_scale_factor = [&]()
    {
        // if the user explicitly enabled high_dpi_mode...
        if (auto v = App::settings().find_value("experimental_feature_flags/high_dpi_mode"); v and *v) {
            return App::get().main_window_dpi() / 96.0f;
        }
        else {
            return 1.0f;  // else: assume it's an unscaled 96dpi screen
        }
    }();

    {
        const std::string base_ini_data = App::slurp("imgui_base_config.ini");
        ImGui::LoadIniSettingsFromMemory(base_ini_data.data(), base_ini_data.size());

        // CARE: the reason this filepath is `static` is because ImGui requires that
        // the string outlives the ImGui context
        static const std::string s_user_imgui_ini_file_path = (App::get().user_data_directory() / "imgui.ini").string();

        ImGui::LoadIniSettingsFromDisk(s_user_imgui_ini_file_path.c_str());
        io.IniFilename = s_user_imgui_ini_file_path.c_str();
    }

    ImFontConfig base_config;
    base_config.SizePixels = dpi_scale_factor*15.0f;
    base_config.PixelSnapH = true;
    base_config.FontDataOwnedByAtlas = true;
    add_resource_as_font(base_config, *io.Fonts, "oscar/fonts/Ruda-Bold.ttf");

    // add FontAwesome icon support
    {
        ImFontConfig config = base_config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = floor(1.5f * config.SizePixels);
        static constexpr auto c_icon_ranges = std::to_array<ImWchar>({ OSC_ICON_MIN, OSC_ICON_MAX, 0 });
        add_resource_as_font(config, *io.Fonts, "oscar/fonts/fa-solid-900.ttf", c_icon_ranges.data());
    }
#endif

    // init ImGui for SDL2 /w OpenGL
    ImGui_ImplSDL2_Init(
        App::upd().upd_underlying_window()
    );

    // init ImGui for OpenGL
    graphics_backend::init();

    apply_dark_theme();

    // init extra parts (plotting, gizmos, etc.)
    ImPlot::CreateContext();
}

void osc::ui::context::shutdown()
{
    ImPlot::DestroyContext();

    graphics_backend::shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool osc::ui::context::on_event(const Event& ev)
{
    ImGui_ImplSDL2_ProcessEvent(&static_cast<const SDL_Event&>(ev));

    const ImGuiIO& io = ImGui::GetIO();

    bool event_handled_by_imgui = false;

    if (io.WantCaptureKeyboard and (ev.type() == EventType::KeyPress or ev.type() == EventType::KeyRelease)) {
        event_handled_by_imgui = true;
    }

    if (io.WantCaptureMouse and (ev.type() == EventType::MouseWheel or ev.type() == EventType::MouseMove or ev.type() == EventType::MouseButtonRelease or ev.type() == EventType::MouseButtonPress)) {
        event_handled_by_imgui = true;
    }

    return event_handled_by_imgui;
}

void osc::ui::context::on_start_new_frame()
{
    graphics_backend::on_start_new_frame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // extra parts
    ImGuizmo::BeginFrame();
}

void osc::ui::context::render()
{
    {
        OSC_PERF("ImGuiRender/render");
        ImGui::Render();
    }

    {
        OSC_PERF("ImGuiRender/ImGui_ImplOscarGfx_RenderDrawData");
        graphics_backend::render(ImGui::GetDrawData());
    }
}
