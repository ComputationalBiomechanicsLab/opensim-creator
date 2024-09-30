#pragma once

#include <oscar/UI/Widgets/SaveChangesPopup.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <filesystem>
#include <vector>
#include <optional>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class MainMenuFileTab final {
    public:
        explicit MainMenuFileTab(Widget&);

        void onDraw(IModelStatePair* = nullptr);

        LifetimedPtr<Widget> m_Parent;
        std::vector<std::filesystem::path> exampleOsimFiles;
        std::optional<SaveChangesPopup> maybeSaveChangesPopup;
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() {}

        void onDraw();
    };
}
