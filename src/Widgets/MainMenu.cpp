#include "MainMenu.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MainEditorState.hpp"
#include "src/OpenSimBindings/StoFileSimulation.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Screens/ExperimentsScreen.hpp"
#include "src/Screens/LoadingScreen.hpp"
#include "src/Screens/MeshImporterScreen.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/Screens/SimulatorScreen.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "osc_config.hpp"

#include <GL/glew.h>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <IconsFontAwesome5.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>

static void doOpenFileViaDialog(std::shared_ptr<osc::MainEditorState> st)
{
    std::filesystem::path p = osc::PromptUserForFile("osim");
    if (!p.empty())
    {
        osc::App::cur().requestTransition<osc::LoadingScreen>(st, p);
    }
}

static std::optional<std::filesystem::path> promptSaveOneFile()
{
    std::filesystem::path p = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("osim");
    return !p.empty() ? std::optional{p} : std::nullopt;
}

static bool isAnExampleFile(std::filesystem::path const& path)
{
    return osc::IsSubpath(osc::App::resource("models"), path);
}

static std::optional<std::string> tryGetModelSaveLocation(OpenSim::Model const& m)
{
    if (std::string const& backing_path = m.getInputFileName();
        backing_path != "Unassigned" && backing_path.size() > 0)
    {
        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (isAnExampleFile(backing_path))
        {
            auto maybePath = promptSaveOneFile();
            return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
        }
        else
        {
            return backing_path;
        }
    }
    else
    {
        // the model has no associated file, so prompt the user for a save
        // location
        auto maybePath = promptSaveOneFile();
        return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
    }
}

static bool trySaveModel(OpenSim::Model const& model, std::string const& save_loc)
{
    try
    {
        model.print(save_loc);
        osc::log::info("saved model to %s", save_loc.c_str());
        return true;
    }
    catch (OpenSim::Exception const& ex)
    {
        osc::log::error("error saving model: %s", ex.what());
        return false;
    }
}

static void transitionToLoadingScreen(std::shared_ptr<osc::MainEditorState> st, std::filesystem::path p)
{
    osc::App::cur().requestTransition<osc::LoadingScreen>(st, p);
}

static void actionSaveCurrentModelAs(osc::UndoableUiModel& uim)
{
    auto maybePath = promptSaveOneFile();

    if (maybePath && trySaveModel(uim.getModel(), maybePath->string()))
    {
        std::string oldPath = uim.getModel().getInputFileName();
        uim.updModel().setInputFileName(maybePath->string());
        uim.setFilesystemPath(*maybePath);
        uim.setUpToDateWithFilesystem();
        if (*maybePath != oldPath)
        {
            uim.commit("set model path");
        }
        osc::App::cur().addRecentFile(*maybePath);
    }
}

template<typename Action>
static void actionCheckToSaveChangesAndThen(osc::MainMenuFileTab& mmft,
                                            std::shared_ptr<osc::MainEditorState> mes,
                                            Action action)
{
    if (!mes)
    {
        action();
    }
    else if (mes && mes->editedModel()->isUpToDateWithFilesystem())
    {
        action();
    }
    else
    {
        osc::SaveChangesPopupConfig cfg;
        cfg.onUserClickedDontSave = [action]()
        {
            action();
            return true;
        };
        cfg.onUserClickedSave = [mes, action]()
        {
            if (osc::actionSaveModel(mes))
            {
                action();
                return true;
            }
            else
            {
                return false;
            }
        };
        mmft.maybeSaveChangesPopup = osc::SaveChangesPopup{std::move(cfg)};
        mmft.maybeSaveChangesPopup->open();
    }
}


// public API

void osc::actionNewModel(std::shared_ptr<MainEditorState> st)
{
    if (st)
    {
        st->editedModel() = std::make_shared<UndoableUiModel>();
        App::cur().requestTransition<ModelEditorScreen>(st);
    }
    else
    {
        App::cur().requestTransition<ModelEditorScreen>(std::make_unique<MainEditorState>());
    }
}

void osc::actionOpenModel(std::shared_ptr<MainEditorState> mes)
{
    doOpenFileViaDialog(mes);
}

bool osc::actionSaveModel(std::shared_ptr<MainEditorState> mes)
{
    std::shared_ptr<UndoableUiModel> uim = mes->editedModel();
    std::optional<std::string> maybeUserSaveLoc = tryGetModelSaveLocation(uim->getModel());

    if (maybeUserSaveLoc && trySaveModel(uim->getModel(), *maybeUserSaveLoc))
    {
        std::string oldPath = uim->getModel().getInputFileName();
        uim->updModel().setInputFileName(*maybeUserSaveLoc);
        uim->setFilesystemPath(*maybeUserSaveLoc);
        uim->setUpToDateWithFilesystem();
        if (*maybeUserSaveLoc != oldPath)
        {
            uim->commit("set model path");
        }
        osc::App::cur().addRecentFile(*maybeUserSaveLoc);
        return true;
    }
    else
    {
        return false;
    }
}

osc::MainMenuFileTab::MainMenuFileTab() :
    exampleOsimFiles{FindAllFilesWithExtensionsRecursively(App::resource("models"), ".osim")},
    recentlyOpenedFiles{App::cur().getRecentFiles()}
{
    std::sort(exampleOsimFiles.begin(), exampleOsimFiles.end(), IsFilenameLexographicallyGreaterThan);
}

void osc::MainMenuFileTab::draw(std::shared_ptr<MainEditorState> mes)
{
    auto& st = *this;

    // handle hotkeys enabled by just drawing the menu
    {
        auto const& io = ImGui::GetIO();
        bool mod = io.KeyCtrl || io.KeySuper;

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_N))
        {
            actionCheckToSaveChangesAndThen(*this, mes, [mes]()
            {
                actionNewModel(mes);
            });
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_O))
        {
            actionCheckToSaveChangesAndThen(*this, mes, [mes]()
            {
                actionOpenModel(mes);
            });
        }

        if (mes && mod && ImGui::IsKeyPressed(SDL_SCANCODE_S))
        {
            actionSaveModel(mes);
        }

        if (mes && mod && io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_S))
        {
            actionSaveCurrentModelAs(*mes->editedModel());
        }

        if (mes && mod && ImGui::IsKeyPressed(SDL_SCANCODE_W))
        {
            actionCheckToSaveChangesAndThen(*this, mes, [mes]()
            {
                App::cur().requestTransition<SplashScreen>();
            });
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_Q))
        {
            actionCheckToSaveChangesAndThen(*this, mes, [mes]()
            {
                App::cur().requestQuit();
            });
        }
    }

    // draw "save as", if necessary
    if (maybeSaveChangesPopup)
    {
        maybeSaveChangesPopup->draw();
    }

    if (!ImGui::BeginMenu("File"))
    {
        return;
    }

    if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N"))
    {
        actionCheckToSaveChangesAndThen(*this, mes, [mes]()
        {
            actionNewModel(mes);
        });
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O"))
    {
        actionCheckToSaveChangesAndThen(*this, mes, [mes]()
        {
            actionOpenModel(mes);
        });
    }

    int imgui_id = 0;

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Recent", !st.recentlyOpenedFiles.empty()))
    {
        // iterate in reverse: recent files are stored oldest --> newest
        for (auto it = st.recentlyOpenedFiles.rbegin(); it != st.recentlyOpenedFiles.rend(); ++it)
        {
            RecentFile const& rf = *it;
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(rf.path.filename().string().c_str()))
            {
                actionCheckToSaveChangesAndThen(*this, mes, [mes, rf]
                {
                    transitionToLoadingScreen(mes, rf.path);
                });
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Example"))
    {
        for (std::filesystem::path const& ex : st.exampleOsimFiles)
        {
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(ex.filename().string().c_str()))
            {
                actionCheckToSaveChangesAndThen(*this, mes, [mes, ex]
                {
                    transitionToLoadingScreen(mes, ex);
                });
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Motion", nullptr, false, mes != nullptr))
    {
        std::filesystem::path p = osc::PromptUserForFile("sto,mot");
        if (!p.empty())
        {
            try
            {
                std::unique_ptr<OpenSim::Model> cpy =
                    std::make_unique<OpenSim::Model>(mes->editedModel()->getModel());
                cpy->buildSystem();
                cpy->initializeState();
                mes->addSimulation(Simulation{osc::StoFileSimulation{std::move(cpy), p}});
                osc::App::cur().requestTransition<osc::SimulatorScreen>(mes);
            }
            catch (std::exception const& ex)
            {
                log::error("encountered error while trying to load an STO file against the model: %s", ex.what());
            }
        }
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save", "Ctrl+S", false, mes != nullptr))
    {
        if (mes)
        {
            actionSaveModel(mes);
        }
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save As", "Shift+Ctrl+S", false, mes != nullptr))
    {
        if (mes)
        {
            actionSaveCurrentModelAs(*mes->editedModel());
        }
    }

    if (ImGui::MenuItem(ICON_FA_MAGIC " Import Meshes"))
    {
        actionCheckToSaveChangesAndThen(*this, mes, []()
        {
            App::cur().requestTransition<MeshImporterScreen>();
        });
    }

    if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W", false, mes != nullptr))
    {
        actionCheckToSaveChangesAndThen(*this, mes, []()
        {
            App::cur().requestTransition<SplashScreen>();
        });
    }

    if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
    {
        actionCheckToSaveChangesAndThen(*this, mes, []()
        {
            App::cur().requestQuit();
        });
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
            int samplesIdx = LeastSignificantBitIndex(App::cur().getMSXAASamplesRecommended());
            int maxSamplesIdx = LeastSignificantBitIndex(App::cur().getMSXAASamplesMax());
            OSC_ASSERT(static_cast<size_t>(maxSamplesIdx) < antialiasingLevels.size());

            if (ImGui::Combo("##msxaa", &samplesIdx, antialiasingLevels.data(), maxSamplesIdx + 1)) {
                App::cur().setMSXAASamplesRecommended(1 << samplesIdx);
            }
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("window");
        ImGui::NextColumn();

        if (ImGui::Button(ICON_FA_EXPAND " fullscreen"))
        {
            App::cur().makeFullscreen();
        }
        if (ImGui::Button(ICON_FA_EXPAND " windowed fullscreen"))
        {
            App::cur().makeWindowedFullscreen();
        }
        if (ImGui::Button(ICON_FA_WINDOW_RESTORE " windowed"))
        {
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

void osc::MainMenuWindowTab::draw(MainEditorState& st)
{

    osc::UserPanelPreferences& panels = st.updUserPanelPrefs();

    // draw "window" tab
    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Actions", nullptr, &panels.actions);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Hierarchy", nullptr, &panels.hierarchy);
        ImGui::MenuItem("Coordinate Editor", nullptr, &panels.coordinateEditor);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Log", nullptr, &panels.log);
        ImGui::MenuItem("Outputs", nullptr, &panels.outputs);
        ImGui::MenuItem("Property Editor", nullptr, &panels.propertyEditor);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Selection Details", nullptr, &panels.selectionDetails);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when simulating a model");
            ImGui::EndTooltip();
        }

        ImGui::MenuItem("Simulations", nullptr, &panels.simulations);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when simulating a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Simulation Stats", nullptr, &panels.simulationStats);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("note: this only shows when editing a model");
            ImGui::EndTooltip();
        }
        ImGui::MenuItem("Perf (development)", nullptr, &panels.perfPanel);
        ImGui::MenuItem("Moment Arm Plotter", nullptr, &panels.momentArmPanel);

        ImGui::Separator();

        // active viewers (can be disabled)
        for (int i = 0; i < st.getNumViewers(); ++i)
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%i", i);

            bool enabled = true;
            if (ImGui::MenuItem(buf, nullptr, &enabled))
            {
                st.removeViewer(i);
                --i;
            }
        }

        if (ImGui::MenuItem("add viewer"))
        {
            st.addViewer();
        }

        ImGui::EndMenu();
    }
}
