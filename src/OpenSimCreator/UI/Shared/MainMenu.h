#pragma once

#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <oscar/UI/Widgets/SaveChangesPopup.h>
#include <oscar/Utils/ParentPtr.h>

#include <filesystem>
#include <vector>
#include <optional>

namespace osc { class IModelStatePair; }

namespace osc
{
    class MainMenuFileTab final {
    public:
        MainMenuFileTab();

        void onDraw(const ParentPtr<IMainUIStateAPI>&, IModelStatePair* = nullptr);

        std::vector<std::filesystem::path> exampleOsimFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() {}

        void onDraw();
    };
}
