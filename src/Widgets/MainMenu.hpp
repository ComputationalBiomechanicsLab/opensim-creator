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

namespace osc
{
    class MainMenuFileTab final {
    public:
        std::vector<std::filesystem::path> exampleOsimFiles;
        std::vector<RecentFile> recentlyOpenedFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;

        MainMenuFileTab();

        void draw(MainUIStateAPI*, UndoableModelStatePair* = nullptr);
    };

    class MainMenuAboutTab final {
    public:
        void draw();
    };
}
