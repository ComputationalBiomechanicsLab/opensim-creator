#pragma once

#include <liboscar/UI/Tabs/Tab.h>
#include <liboscar/UI/Tabs/TabSaveResult.h>

#include <filesystem>
#include <future>
#include <vector>

namespace osc { class Widget; }

namespace osc::mi
{
    class MeshImporterTab final : public Tab {
    public:
        static CStringView id() { return "OpenSim/MeshImporter"; }

        explicit MeshImporterTab(Widget*);
        explicit MeshImporterTab(Widget*, std::vector<std::filesystem::path>);

    private:
        bool impl_is_unsaved() const final;
        std::future<TabSaveResult> impl_try_save() final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
