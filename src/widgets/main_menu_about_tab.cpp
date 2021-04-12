#include "main_menu_about_tab.hpp"

#include "osmv_config.hpp"
#include "src/application.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/opengl_test_screen.hpp"
#include "src/utils/bitwise_algs.hpp"
#include "src/widgets/help_marker.hpp"

#include <GL/glew.h>
#include <imgui.h>

#include <array>

static void draw_header(char const* str) {
    ImGui::Text("%s", str);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
}

void osc::draw_main_menu_about_tab() {
    if (!ImGui::BeginMenu("About")) {
        return;
    }

    static constexpr float menu_width = 400;
    ImGui::Dummy(ImVec2(menu_width, 0));

    draw_header("graphics");
    {
        ImGui::Columns(2);

        ImGui::Text("FPS");
        ImGui::NextColumn();
        ImGui::Text("%.1f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::NextColumn();

        ImGui::Text("MSXAA");
        ImGui::NextColumn();
        {
            static constexpr std::array<char const*, 8> aa_lvls = {"x1", "x2", "x4", "x8", "x16", "x32", "x64", "x128"};
            int samples_idx = lsb_index(Application::current().samples());
            int max_samples_idx = lsb_index(Application::current().max_samples());
            OSC_ASSERT(static_cast<size_t>(max_samples_idx) < aa_lvls.size());

            if (ImGui::Combo("##msxaa", &samples_idx, aa_lvls.data(), max_samples_idx + 1)) {
                Application::current().set_samples(1 << samples_idx);
            }
        }
        ImGui::NextColumn();

        ImGui::Text("window");
        ImGui::NextColumn();

        if (ImGui::Button("fullscreen")) {
            Application::current().make_fullscreen();
        }
        ImGui::SameLine();
        if (ImGui::Button("windowed")) {
            Application::current().make_windowed();
        }
        ImGui::NextColumn();

        ImGui::Text("VSYNC");
        ImGui::NextColumn();
        if (ImGui::Button("enable")) {
            Application::current().enable_vsync();
        }
        ImGui::SameLine();
        if (ImGui::Button("disable")) {
            Application::current().disable_vsync();
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    draw_header("properties");
    {
        ImGui::Columns(2);

        ImGui::Text("OSC_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", OSC_VERSION_STRING);
        ImGui::NextColumn();

        ImGui::Text("OSC_BUILD_ID");
        ImGui::NextColumn();
        ImGui::Text("%s", OSC_BUILD_ID);
        ImGui::NextColumn();

        ImGui::Text("GL_VENDOR");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VENDOR));
        ImGui::NextColumn();

        ImGui::Text("GL_RENDERER");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_RENDERER));
        ImGui::NextColumn();

        ImGui::Text("GL_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VERSION));
        ImGui::NextColumn();

        ImGui::Text("GL_SHADING_LANGUAGE_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    draw_header("utils");
    {
        ImGui::Columns(2);

        ImGui::Text("ImGui demo");
        ImGui::SameLine();
        draw_help_marker(
            "shows the standard ImGui demo screen (ImGui::ShowDemoWindow). Useful for finding an ImGui feature.");
        ImGui::NextColumn();
        int id = 0;
        ImGui::PushID(id++);
        if (ImGui::Button("show")) {
            Application::current().request_screen_transition<Imgui_demo_screen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Text("OpenGL experiments");
        ImGui::SameLine();
        draw_help_marker(
            "opens a test screen for low-level OpenGL features - you probably don't care about this, but it's useful for testing hardware features in prod");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button("show")) {
            Application::current().request_screen_transition<Opengl_test_screen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Text("Debug mode");
        ImGui::SameLine();
        draw_help_marker(
            "Toggles whether the application is in debug mode or not: enabling this can reveal more inforamtion about bugs");
        ImGui::NextColumn();
        {
            Application& app = Application::current();
            bool debug_mode = app.is_in_debug_mode();
            if (ImGui::Checkbox("##opengldebugmodecheckbox", &debug_mode)) {
                if (debug_mode) {
                    app.enable_debug_mode();
                } else {
                    app.disable_debug_mode();
                }
            }
        }

        ImGui::Columns();
    }

    ImGui::EndMenu();
}
