#pragma once

#include "src/Platform/RecentFile.hpp"
#include "src/Widgets/SaveChangesPopup.hpp"

#include <filesystem>
#include <memory>
#include <vector>
#include <optional>

namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class MainMenuFileTab final {
    public:
        MainMenuFileTab();

        void draw(std::weak_ptr<MainUIStateAPI>, UndoableModelStatePair* = nullptr);

        std::vector<std::filesystem::path> exampleOsimFiles;
        std::vector<RecentFile> recentlyOpenedFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;
    };

    class MainMenuAboutTab final {
    public:
        void draw();
    };
}
