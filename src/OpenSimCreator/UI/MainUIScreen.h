#pragma once

#include <oscar/Platform/Screen.h>
#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/ParentPtr.h>
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

        UID addTab(std::unique_ptr<Tab>);
        void open(const std::filesystem::path&);

        template<std::derived_from<Tab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        UID add_tab(Args&&... args)
        {
            return add_tab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        UID add_tab(std::unique_ptr<Tab>);
        void select_tab(UID);
        void close_tab(UID);
        void reset_imgui();

        template<std::derived_from<Tab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        void add_and_select_tab(Args&&... args)
        {
            const UID tab_id = add_tab<T>(std::forward<Args>(args)...);
            select_tab(tab_id);
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

        operator ParentPtr<MainUIScreen> ();

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
