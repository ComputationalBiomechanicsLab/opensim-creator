#pragma once

#include <liboscar/platform/widget.h>
#include <liboscar/ui/popups/save_changes_popup.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Widget;

    class MainMenuFileTab final : public Widget {
    public:
        explicit MainMenuFileTab(Widget* parent);

        void on_draw(std::shared_ptr<opyn::ModelStatePair> = nullptr);

        std::vector<std::filesystem::path> example_osim_files;

    private:
        void impl_on_draw() override { on_draw(); }
    };

    class MainMenuAboutTab final {
    public:
        MainMenuAboutTab() = default;

        void on_draw();
    };
}
