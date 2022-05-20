#include "MainMenu.hpp"

#include "osc_config.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/OpenSimBindings/StoFileSimulation.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Screens/ExperimentsScreen.hpp"
#include "src/Tabs/LoadingTab.hpp"
#include "src/Tabs/MeshImporterTab.hpp"
#include "src/Tabs/ModelEditorTab.hpp"
#include "src/Tabs/SimulatorTab.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/MainUIStateAPI.hpp"

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


static void OpenOsimInLoadingTab(osc::MainUIStateAPI* api, std::filesystem::path p)
{
    osc::UID tabID = api->addTab<osc::LoadingTab>(api, p);
    api->selectTab(tabID);
}

static void DoOpenFileViaDialog(osc::MainUIStateAPI* api)
{
    std::filesystem::path p = osc::PromptUserForFile("osim");
    if (!p.empty())
    {
        OpenOsimInLoadingTab(api, p);
    }
}

static std::optional<std::filesystem::path> PromptSaveOneFile()
{
    std::filesystem::path p = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("osim");
    return !p.empty() ? std::optional{p} : std::nullopt;
}

static bool IsAnExampleFile(std::filesystem::path const& path)
{
    return osc::IsSubpath(osc::App::resource("models"), path);
}

static std::optional<std::string> TryGetModelSaveLocation(OpenSim::Model const& m)
{
    if (std::string const& backing_path = m.getInputFileName();
        backing_path != "Unassigned" && backing_path.size() > 0)
    {
        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (IsAnExampleFile(backing_path))
        {
            auto maybePath = PromptSaveOneFile();
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
        auto maybePath = PromptSaveOneFile();
        return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
    }
}

static bool TrySaveModel(OpenSim::Model const& model, std::string const& save_loc)
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

static void ActionSaveCurrentModelAs(osc::UndoableModelStatePair& uim)
{
    auto maybePath = PromptSaveOneFile();

    if (maybePath && TrySaveModel(uim.getModel(), maybePath->string()))
    {
        std::string oldPath = uim.getModel().getInputFileName();
        uim.updModel().setInputFileName(maybePath->string());
        uim.setFilesystemPath(*maybePath);
        uim.setUpToDateWithFilesystem();
        if (*maybePath != oldPath)
        {
            uim.commit("set model path");
        }
        osc::App::upd().addRecentFile(*maybePath);
    }
}

template<typename Action>
static void actionCheckToSaveChangesAndThen(osc::MainMenuFileTab& mmft, osc::MainUIStateAPI* api, osc::UndoableModelStatePair* model, Action action)
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

void osc::actionNewModel(MainUIStateAPI* api)
{

    auto p = std::make_unique<UndoableModelStatePair>();
    UID tabID = api->addTab<ModelEditorTab>(api, std::move(p));
    api->selectTab(tabID);
}

void osc::actionOpenModel(MainUIStateAPI* api)
{
    DoOpenFileViaDialog(api);
}

bool osc::actionSaveModel(MainUIStateAPI* api, UndoableModelStatePair& model)
{
    std::optional<std::string> maybeUserSaveLoc = TryGetModelSaveLocation(model.getModel());

    if (maybeUserSaveLoc && TrySaveModel(model.getModel(), *maybeUserSaveLoc))
    {
        std::string oldPath = model.getModel().getInputFileName();
        model.updModel().setInputFileName(*maybeUserSaveLoc);
        model.setFilesystemPath(*maybeUserSaveLoc);
        model.setUpToDateWithFilesystem();
        if (*maybeUserSaveLoc != oldPath)
        {
            model.commit("set model path");
        }
        osc::App::upd().addRecentFile(*maybeUserSaveLoc);
        return true;
    }
    else
    {
        return false;
    }
}

osc::MainMenuFileTab::MainMenuFileTab() :
    exampleOsimFiles{FindAllFilesWithExtensionsRecursively(App::resource("models"), ".osim")},
    recentlyOpenedFiles{App::get().getRecentFiles()}
{
    std::sort(exampleOsimFiles.begin(), exampleOsimFiles.end(), IsFilenameLexographicallyGreaterThan);
}

void osc::MainMenuFileTab::draw(MainUIStateAPI* api, UndoableModelStatePair* maybeModel)
{
    // handle hotkeys enabled by just drawing the menu
    {
        auto const& io = ImGui::GetIO();
        bool mod = io.KeyCtrl || io.KeySuper;

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_N))
        {
            actionNewModel(api);
        }

        if (mod && ImGui::IsKeyPressed(SDL_SCANCODE_O))
        {
            actionOpenModel(api);
        }

        if (maybeModel && mod && ImGui::IsKeyPressed(SDL_SCANCODE_S))
        {
            actionSaveModel(api, *maybeModel);
        }

        if (maybeModel && mod && io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_S))
        {
            actionSaveModel(api, *maybeModel);
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
        actionNewModel(api);
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O"))
    {
        actionOpenModel(api);
    }

    int imgui_id = 0;

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Recent", !recentlyOpenedFiles.empty()))
    {
        // iterate in reverse: recent files are stored oldest --> newest
        for (auto it = recentlyOpenedFiles.rbegin(); it != recentlyOpenedFiles.rend(); ++it)
        {
            RecentFile const& rf = *it;
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(rf.path.filename().string().c_str()))
            {
                OpenOsimInLoadingTab(api, rf.path);
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " Open Example"))
    {
        for (std::filesystem::path const& ex : exampleOsimFiles)
        {
            ImGui::PushID(++imgui_id);
            if (ImGui::MenuItem(ex.filename().string().c_str()))
            {
                OpenOsimInLoadingTab(api, ex);
            }
            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Motion", nullptr, false, maybeModel != nullptr))
    {
        std::filesystem::path p = osc::PromptUserForFile("sto,mot");
        if (!p.empty())
        {
            try
            {
                std::unique_ptr<OpenSim::Model> cpy = std::make_unique<OpenSim::Model>(maybeModel->getModel());
                cpy->buildSystem();
                cpy->initializeState();

                UID tabID = api->addTab<SimulatorTab>(api, std::make_shared<Simulation>(osc::StoFileSimulation{std::move(cpy), p}));
                api->selectTab(tabID);
            }
            catch (std::exception const& ex)
            {
                log::error("encountered error while trying to load an STO file against the model: %s", ex.what());
            }
        }
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save", "Ctrl+S", false, maybeModel != nullptr))
    {
        actionSaveModel(api, *maybeModel);
    }

    if (ImGui::MenuItem(ICON_FA_SAVE " Save As", "Shift+Ctrl+S", false, maybeModel != nullptr))
    {
        ActionSaveCurrentModelAs(*maybeModel);
    }

    if (ImGui::MenuItem(ICON_FA_MAGIC " Import Meshes"))
    {
        UID tabID = api->addTab<MeshImporterTab>(api);
    }

    if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
    {
        App::upd().requestQuit();
    }

    ImGui::EndMenu();
}


void osc::MainMenuAboutTab::draw()
{
    if (!ImGui::BeginMenu("About"))
    {
        return;
    }

    static constexpr float menuWidth = 400;
    ImGui::Dummy({menuWidth, 0});

    ImGui::TextUnformatted("graphics");
    ImGui::SameLine();
    DrawHelpMarker("OSMV's global graphical settings");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 0.5f});
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
            int samplesIdx = LeastSignificantBitIndex(App::get().getMSXAASamplesRecommended());
            int maxSamplesIdx = LeastSignificantBitIndex(App::get().getMSXAASamplesMax());
            OSC_ASSERT(static_cast<size_t>(maxSamplesIdx) < antialiasingLevels.size());

            if (ImGui::Combo("##msxaa", &samplesIdx, antialiasingLevels.data(), maxSamplesIdx + 1))
            {
                App::upd().setMSXAASamplesRecommended(1 << samplesIdx);
            }
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("window");
        ImGui::NextColumn();

        if (ImGui::Button(ICON_FA_EXPAND " fullscreen"))
        {
            App::upd().makeFullscreen();
        }
        if (ImGui::Button(ICON_FA_EXPAND " windowed fullscreen"))
        {
            App::upd().makeWindowedFullscreen();
        }
        if (ImGui::Button(ICON_FA_WINDOW_RESTORE " windowed"))
        {
            App::upd().makeWindowed();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("VSYNC");
        ImGui::SameLine();
        DrawHelpMarker("whether the backend uses vertical sync (VSYNC), which will cap the rendering FPS to your monitor's refresh rate");
        ImGui::NextColumn();

        bool enabled = App::get().isVsyncEnabled();
        if (ImGui::Checkbox("##vsynccheckbox", &enabled)) {
            if (enabled) {
                App::upd().enableVsync();
            } else {
                App::upd().disableVsync();
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    ImGui::Dummy({0.0f, 2.0f});
    ImGui::TextUnformatted("properties");
    ImGui::SameLine();
    DrawHelpMarker("general software properties: useful information for bug reporting etc.");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 0.5f});
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

    ImGui::Dummy({0.0f, 2.5f});
    ImGui::TextUnformatted("debugging utilities:");
    ImGui::SameLine();
    DrawHelpMarker("standard utilities that can help with development, debugging, etc.");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 0.5f});
    int id = 0;
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("Experimental Screens");
        ImGui::SameLine();
        DrawHelpMarker("opens a test screen for experimental features - you probably don't care about this, but it's useful for testing hardware features in prod");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_EYE " show"))
        {
            App::upd().requestTransition<ExperimentsScreen>();
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::TextUnformatted("OSC Install Location");
        ImGui::SameLine();
        DrawHelpMarker("opens OSC's installation location in your OS's default file browser");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_FOLDER " open"))
        {
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
            bool appIsInDebugMode = App::get().isInDebugMode();
            if (ImGui::Checkbox("##opengldebugmodecheckbox", &appIsInDebugMode))
            {
                if (appIsInDebugMode)
                {
                    App::upd().enableDebugMode();
                }
                else
                {
                    App::upd().disableDebugMode();
                }
            }
        }

        ImGui::Columns();
    }

    ImGui::Dummy({0.0f, 2.5f});
    ImGui::TextUnformatted("useful links:");
    ImGui::SameLine();
    DrawHelpMarker("links to external sites that might be useful");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 0.5f});
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("OpenSim Creator Documentation");
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button(ICON_FA_LINK " open")) {
            OpenPathInOSDefaultApplication(App::get().getConfig().getHTMLDocsDir() / "index.html");
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
