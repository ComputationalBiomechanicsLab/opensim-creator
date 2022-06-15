#pragma once

#include "src/Platform/RecentFile.hpp"
#include "src/Widgets/SaveChangesPopup.hpp"

#include <filesystem>
#include <vector>
#include <optional>

namespace osc
{
    class MainUIStateAPI;
    class UndoableModelStatePair;
}

namespace OpenSim
{
    class Model;
}

namespace osc
{
    struct MainMenuFileTab final {
        std::vector<std::filesystem::path> exampleOsimFiles;
        std::vector<RecentFile> recentlyOpenedFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;

        MainMenuFileTab();

        void draw(MainUIStateAPI*, UndoableModelStatePair* = nullptr);
    };

    struct MainMenuAboutTab final {
        void draw();
    };
}
