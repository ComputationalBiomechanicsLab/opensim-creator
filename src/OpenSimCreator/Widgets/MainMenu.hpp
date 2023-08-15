#pragma once

#include <oscar/Platform/RecentFile.hpp>
#include <oscar/Widgets/SaveChangesPopup.hpp>

#include <filesystem>
#include <memory>
#include <vector>
#include <optional>

namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class MainMenuFileTab final {
    public:
        MainMenuFileTab();

        void onDraw(ParentPtr<MainUIStateAPI> const&, UndoableModelStatePair* = nullptr);

        std::vector<std::filesystem::path> exampleOsimFiles;
        std::vector<RecentFile> recentlyOpenedFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() {}

        void onDraw();
    };
}
