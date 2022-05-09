#pragma once

#include "src/Platform/RecentFile.hpp"
#include "src/Widgets/SaveChangesPopup.hpp"

#include <filesystem>
#include <vector>
#include <memory>
#include <optional>

namespace osc
{
    class MainEditorState;
}

namespace OpenSim
{
    class Model;
}

namespace osc
{
    void actionNewModel(std::shared_ptr<MainEditorState>);
    void actionOpenModel(std::shared_ptr<MainEditorState>);
    bool actionSaveModel(std::shared_ptr<MainEditorState>);

    struct MainMenuFileTab final {
        std::vector<std::filesystem::path> exampleOsimFiles;
        std::vector<RecentFile> recentlyOpenedFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;

        MainMenuFileTab();

        void draw(std::shared_ptr<MainEditorState>);
    };

    struct MainMenuAboutTab final {
        void draw();
    };
}
