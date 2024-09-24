#pragma once

#include <oscar/Platform/IScreen.h>
#include <oscar/Utils/UID.h>

#include <filesystem>
#include <memory>

namespace osc { class ITab; }
namespace osc { class ITabHost; }

namespace osc
{
    class MainUIScreen final : public IScreen {
    public:
        MainUIScreen();
        MainUIScreen(const MainUIScreen&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept;
        MainUIScreen& operator=(const MainUIScreen&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept;
        ~MainUIScreen() noexcept override;

        UID addTab(std::unique_ptr<ITab>);
        void open(const std::filesystem::path&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
