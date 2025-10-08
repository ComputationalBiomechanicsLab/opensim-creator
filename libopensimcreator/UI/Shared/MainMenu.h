#pragma once

#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/Popups/SaveChangesPopup.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class MainMenuFileTab final : public Widget {
    public:
        explicit MainMenuFileTab(Widget* parent);

        void onDraw(std::shared_ptr<IModelStatePair> = nullptr);

        std::vector<std::filesystem::path> exampleOsimFiles;

    private:
        void impl_on_draw() final { onDraw(); }
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() {}

        void onDraw();
    };
}
