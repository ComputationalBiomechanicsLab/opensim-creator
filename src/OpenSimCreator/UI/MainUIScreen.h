#pragma once

#include <oscar/Platform/Screen.h>
#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/LifetimedPtr.h>
#include <oscar/Utils/UID.h>

#include <concepts>
#include <filesystem>
#include <memory>
#include <utility>

namespace osc { class OutputExtractor; }
namespace osc { class ParamBlock; }

namespace osc
{
    class MainUIScreen final : public Screen {
    public:
        MainUIScreen();
        MainUIScreen(const MainUIScreen&) = delete;
        MainUIScreen(MainUIScreen&&) noexcept;
        MainUIScreen& operator=(const MainUIScreen&) = delete;
        MainUIScreen& operator=(MainUIScreen&&) noexcept;
        ~MainUIScreen() noexcept override;

        void open(const std::filesystem::path&);

        void close_tab(UID);

        template<std::derived_from<Tab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        UID add_and_select_tab(Args&&... args)
        {
            return add_and_select_tab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        const ParamBlock& getSimulationParams() const;
        ParamBlock& updSimulationParams();

        int getNumUserOutputExtractors() const;
        const OutputExtractor& getUserOutputExtractor(int index) const;
        void addUserOutputExtractor(const OutputExtractor& extractor);
        void removeUserOutputExtractor(int index);
        bool hasUserOutputExtractor(const OutputExtractor& extractor) const;
        bool removeUserOutputExtractor(const OutputExtractor& extractor);
        bool overwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer);

        LifetimedPtr<MainUIScreen> weak_ref()
        {
            return Widget::weak_ref().dynamic_downcast<MainUIScreen>();
        }

    private:
        UID add_and_select_tab(std::unique_ptr<Tab>);

        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
