#pragma once

#include <OpenSimCreator/UI/MainUIScreen.h>

#include <oscar/UI/Widgets/SaveChangesPopup.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <filesystem>
#include <vector>
#include <optional>

namespace osc { class IModelStatePair; }

namespace osc
{
    class MainMenuFileTab final {
    public:
        explicit MainMenuFileTab(MainUIScreen&);

        void onDraw(IModelStatePair* = nullptr);

        LifetimedPtr<MainUIScreen> m_Parent;
        std::vector<std::filesystem::path> exampleOsimFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() {}

        void onDraw();
    };
}
