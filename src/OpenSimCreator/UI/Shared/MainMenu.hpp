#pragma once

#include <oscar/UI/Widgets/SaveChangesPopup.hpp>

#include <filesystem>
#include <memory>
#include <vector>
#include <optional>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class MainMenuFileTab final {
    public:
        MainMenuFileTab();

        void onDraw(ParentPtr<IMainUIStateAPI> const&, UndoableModelStatePair* = nullptr);

        std::vector<std::filesystem::path> exampleOsimFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() {}

        void onDraw();
    };
}
