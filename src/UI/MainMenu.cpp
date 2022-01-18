#include "MainMenu.hpp"

#include "src/3D/Gl.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Screens/Experimental/ImGuiDemoScreen.hpp"
#include "src/Screens/LoadingScreen.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"
#include "src/Screens/MeshImporterScreen.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/MainEditorState.hpp"
#include "src/App.hpp"
#include "src/Config.hpp"
#include "src/Log.hpp"
#include "src/os.hpp"
#include "src/Styling.hpp"
#include "osc_config.hpp"

#include <imgui.h>
#include <nfd.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using namespace osc;

static void doOpenFileViaDialog(std::shared_ptr<MainEditorState> st) {
    nfdchar_t* outpath = nullptr;

    nfdresult_t result = NFD_OpenDialog("osim", nullptr, &outpath);
    OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    if (result == NFD_OKAY) {
        App::cur().requestTransition<LoadingScreen>(st, outpath);
    }
}

static std::optional<std::filesystem::path> promptSaveOneFile() {
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_SaveDialog("osim", nullptr, &outpath);
    OSC_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
}

static bool isAnExampleFile(std::filesystem::path const& path) {
    return IsSubpath(App::resource("models"), path);
}

static std::optional<std::string> tryGetModelSaveLocation(OpenSim::Model const& m) {
    if (std::string const& backing_path = m.getInputFileName();
        backing_path != "Unassigned" && backing_path.size() > 0) {

        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (isAnExampleFile(backing_path)) {
            auto maybePath = promptSaveOneFile();
            return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
        } else {
            return backing_path;
        }
    } else {
        // the model has no associated file, so prompt the user for a save
        // location
        auto maybePath = promptSaveOneFile();
        return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
    }
}

static bool trySaveModel(OpenSim::Model const& model, std::string const& save_loc) {
    try {
        model.print(save_loc);
        log::info("saved model to %s", save_loc.c_str());
        return true;
    } catch (OpenSim::Exception const& ex) {
        log::error("error saving model: %s", ex.what());
        return false;
    }
}

static void transitionToLoadingScreen(std::shared_ptr<MainEditorState> st, std::filesystem::path p) {
    App::cur().requestTransition<LoadingScreen>(st, p);
}

static void actionSaveCurrentModel(UiModel& uim) {
    std::optional<std::string> maybeUserSaveLoc = tryGetModelSaveLocation(uim.getModel());

    if (maybeUserSaveLoc && trySaveModel(uim.getModel(), *maybeUserSaveLoc)) {
        uim.updModel().setInputFileName(*maybeUserSaveLoc);
        App::cur().addRecentFile(*maybeUserSaveLoc);
    }
}

static void actionSaveCurrentModelAs(UiModel& uim) {
    auto maybePath = promptSaveOneFile();

    if (maybePath && trySaveModel(uim.getModel(), maybePath->string())) {
        uim.updModel().setInputFileName(maybePath->string());
        App::cur().addRecentFile(*maybePath);
    }
}


// public API

void osc::actionNewModel(std::shared_ptr<MainEditorState> st) {
    if (st) {
        st->editedModel = UndoableUiModel{};
        App::cur().requestTransition<ModelEditorScreen>(st);
    } else {
        App::cur().requestTransition<ModelEditorScreen>(std::make_unique<MainEditorState>());
    }
}

void osc::actionOpenModel(std::shared_ptr<MainEditorState> mes) {
    doOpenFileViaDialog(mes);
}

osc::MainMenuFileTab::MainMenuFileTab() :
    exampleOsimFiles{FindAllFilesWithExtensionsRecursively(App::resource("models"), ".osim")},
    recentlyOpenedFiles{App::cur().getRecentFiles()} {

    std::sort(exampleOsimFiles.begin(), exampleOsimFiles.end(), IsFilenameLexographicallyGreaterThan);
}

void osc::MainMenuFileTab::draw(std::shared_ptr<MainEditorState> editor_state) {
    auto& st = *this;

    // handle hotkeys enabled by just drawing the menu
    {
        auto const& io = ImGui::GetIO();
        bool mod = io.KeyCtrl || io.KeySuper;

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_N)) {
            actionNewModel(editor_state);
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_O)) {
            actionOpenModel(editor_state);
        }

        if (editor_state && mod && ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
            actionSaveCurrentModel(editor_state->editedModel.updUiModel());
        }

        if (editor_state && mod && io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
            actionSaveCurrentModelAs(editor_state->editedModel.updUiModel());
        }

        if (editor_state && mod && ImGui::IsKeyPressed(SDL_SCANCODE_W)) {
            App::cur().requestTransition<SplashScreen>();
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_Q)) {
            App::cur().requestQuit();
        }
    }

    if (!ImGui::BeginMenu("File")) {
        return;
    }

    if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N")) {
        actionNewModel(editor_state);
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O")) {
        actionOpenModel(editor_state);
    }

    int imgui_id = 0;

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Recent", !st.recentlyOpenedFiles.empty())) {
        // iterate in reverse: recent files are stored oldest --> newest
        for (auto it = st.recentlyOpenedFiles.rbegin(); it != st.recentlyOpenedFiles.rend(); ++it) {
            RecentFile const& rf = *it;
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(rf.path.filename().string().c_str())) {
                transitionToLoadingScreen(editor_state, rf.path);
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Example")) {
        for (std::filesystem::path const& ex : st.exampleOsimFiles) {
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(ex.filename().string().c_str())) {
                transitionToLoadingScreen(editor_state, ex);
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save", "Ctrl+S", false, editor_state != nullptr)) {
        if (editor_state) {
            actionSaveCurrentModel(editor_state->editedModel.updUiModel());
        }
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save As", "Shift+Ctrl+S", false, editor_state != nullptr)) {
        if (editor_state) {
            actionSaveCurrentModelAs(editor_state->editedModel.updUiModel());
        }
    }

    if (ImGui::MenuItem(ICON_FA_MAGIC " Import Meshes")) {
        App::cur().requestTransition<MeshImporterScreen>();
    }

    if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W", false, editor_state != nullptr)) {
        App::cur().requestTransition<SplashScreen>();
    }

    if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q")) {
        App::cur().requestQuit();
    }
    ImGui::EndMenu();
}


void osc::MainMenuAboutTab::draw() {
    if (!ImGui::BeginMenu("About")) {
        return;
    }

    static constexpr float menuWidth = 400;
    ImGui::Dummy(ImVec2(menuWidth, 0));

    ImGui::TextUnformatted("graphics");
    ImGui::SameLine();
    DrawHelpMarker("OSMV's global graphical settings");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("FPS");
        ImGui::NextColumn();
        ImGui::Text("%.0f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::NextColumn();

        ImGui::TextUnformatted("MSXAA");
        ImGui::SameLine();
        DrawHelpMarker("the level of MultiSample Anti-Aliasing to use. This only affects 3D renders *within* the UI, not the whole UI (panels etc. will not be affected)");
        ImGui::NextColumn();
        {
            static constexpr std::array<char const*, 8> antialiasingLevels = {"x1", "x2", "x4", "x8", "x16", "x32", "x64", "x128"};
            int samplesIdx = LeastSignificantBitIndex(App::cur().getSamples());
            int maxSamplesIdx = LeastSignificantBitIndex(App::cur().maxSamples());
            OSC_ASSERT(static_cast<size_t>(maxSamplesIdx) < antialiasingLevels.size());

            if (ImGui::Combo("##msxaa", &samplesIdx, antialiasingLevels.data(), maxSamplesIdx + 1)) {
                App::cur().setSamples(1 << samplesIdx);
            }
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("window");
        ImGui::NextColumn();

        if (ImGui::Button(ICON_FA_EXPAND " fullscreen")) {
            App::cur().makeFullscreen();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_WINDOW_RESTORE " windowed")) {
            App::cur().makeWindowed();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("VSYNC");
        ImGui::SameLine();
        DrawHelpMarker("whether the backend uses vertical sync (VSYNC), which will cap the rendering FPS to your monitor's refresh rate");
        ImGui::NextColumn();

        bool enabled = App::cur().isVsyncEnabled();
        if (ImGui::Checkbox("##vsynccheckbox", &enabled)) {
            if (enabled) {
                App::cur().enableVsync();
            } else {
                App::cur().disableVsync();
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::TextUnformatted("properties");
    ImGui::SameLine();
    DrawHelpMarker("general software properties: useful information for bug reporting etc.");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("OSC_VERSION");
        ImGui::NextColumn();
        ImGui::TextUnformatted(OSC_VERSION_STRING);
        ImGui::NextColumn();

        ImGui::TextUnformatted("OSC_BUILD_ID");
        ImGui::NextColumn();
        ImGui::TextUnformatted(OSC_BUILD_ID);
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_VENDOR");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VENDOR));
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_RENDERER");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_RENDERER));
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_VERSION));
        ImGui::NextColumn();

        ImGui::TextUnformatted("GL_SHADING_LANGUAGE_VERSION");
        ImGui::NextColumn();
        ImGui::Text("%s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    ImGui::Dummy(ImVec2(0.0f, 2.5f));
    ImGui::TextUnformatted("debugging utilities:");
    ImGui::SameLine();
    DrawHelpMarker("standard utilities that can help with development, debugging, etc.");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    int id = 0;
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("Experimental Screens");
        ImGui::SameLine();
        DrawHelpMarker("opens a test screen for experimental features - you probably don't care about this, but it's useful for testing hardware features in prod");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_EYE " show")) {
            App::cur().requestTransition<ExperimentsScreen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("OSC Install Location");
        ImGui::SameLine();
        DrawHelpMarker("opens OSC's installation location in your OS's default file browser");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_FOLDER " open")) {
            OpenPathInOSDefaultApplication(CurrentExeDir());
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("User Data Dir");
        ImGui::SameLine();
        DrawHelpMarker("opens your OSC user data directory in your OS's default file browser");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_FOLDER " open")) {
            OpenPathInOSDefaultApplication(GetUserDataDir());
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("Debug mode");
        ImGui::SameLine();
        DrawHelpMarker("Toggles whether the application is in debug mode or not: enabling this can reveal more inforamtion about bugs");
        ImGui::NextColumn();
        {
            App& app = App::cur();
            bool appIsInDebugMode = app.isInDebugMode();
            if (ImGui::Checkbox("##opengldebugmodecheckbox", &appIsInDebugMode)) {
                if (appIsInDebugMode) {
                    app.enableDebugMode();
                } else {
                    app.disableDebugMode();
                }
            }
        }

        ImGui::Columns();
    }

    ImGui::Dummy(ImVec2(0.0f, 2.5f));
    ImGui::TextUnformatted("useful links:");
    ImGui::SameLine();
    DrawHelpMarker("links to external sites that might be useful");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 0.5f));
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("OpenSim Creator Documentation");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_LINK " open")) {
            OpenPathInOSDefaultApplication(App::config().htmlDocsDir / "index.html");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("this will open the (locally installed) documentation in a separate browser window");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("OpenSim Creator GitHub");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_LINK " open")) {
            OpenPathInOSDefaultApplication(OSC_REPO_URL);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("this will open the GitHub homepage in a separate browser window");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("OpenSim Documentation");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_LINK " open")) {
            OpenPathInOSDefaultApplication("https://simtk-confluence.stanford.edu/display/OpenSim/Documentation");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("this will open the documentation in a separate browser window");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::EndMenu();
}

void osc::MainMenuWindowTab::draw(MainEditorState& st) {
    // draw "window" tab
    if (ImGui::BeginMenu("Window")) {

        ImGui::MenuItem("Actions", nullptr, &st.showing.actions);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Hierarchy", nullptr, &st.showing.hierarchy);
        ImGui::MenuItem("Coordinate Editor", nullptr, &st.showing.coordinateEditor);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Log", nullptr, &st.showing.log);
        ImGui::MenuItem("Outputs", nullptr, &st.showing.outputs);
        ImGui::MenuItem("Property Editor", nullptr, &st.showing.propertyEditor);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Selection Details", nullptr, &st.showing.selectionDetails);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when simulating a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Simulations", nullptr, &st.showing.simulations);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when simulating a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Simulation Stats", nullptr, &st.showing.simulationStats);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }

        for (size_t i = 0; i < st.viewers.size(); ++i) {
            UiModelViewer* viewer = st.viewers[i].get();

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%zu", i);

            bool enabled = viewer != nullptr;
            if (ImGui::MenuItem(buf, nullptr, &enabled)) {
                if (enabled) {
                    // was enabled by user click
                    st.viewers[i] = std::make_unique<UiModelViewer>();
                } else {
                    // was disabled by user click
                    st.viewers[i] = nullptr;
                }
            }
        }

        ImGui::EndMenu();
    }
}
