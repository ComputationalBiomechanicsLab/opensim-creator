#pragma once

#include <liboscar/ui/tabs/tab.h>
#include <liboscar/ui/tabs/tab_save_result.h>

#include <filesystem>
#include <future>
#include <vector>

namespace osc
{
    class Widget;

    class MeshImporterTab final : public Tab {
    public:
        static CStringView id() { return "OpenSim/MeshImporter"; }

        explicit MeshImporterTab(Widget*);
        explicit MeshImporterTab(Widget*, std::vector<std::filesystem::path>);

    private:
        bool impl_is_unsaved() const override;
        std::future<TabSaveResult> impl_try_save() override;
        void impl_on_mount() override;
        void impl_on_unmount() override;
        bool impl_on_event(Event&) override;
        void impl_on_tick() override;
        void impl_on_draw_main_menu() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
