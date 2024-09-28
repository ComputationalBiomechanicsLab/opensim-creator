#pragma once

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/ParentPtr.h>

#include <filesystem>
#include <vector>

namespace osc { class IMainUIStateAPI; }

namespace osc::mi
{
    class MeshImporterTab final : public Tab {
    public:
        explicit MeshImporterTab(const ParentPtr<IMainUIStateAPI>&);
        MeshImporterTab(const ParentPtr<IMainUIStateAPI>&, std::vector<std::filesystem::path>);

    private:
        bool impl_is_unsaved() const final;
        bool impl_try_save() final;
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
