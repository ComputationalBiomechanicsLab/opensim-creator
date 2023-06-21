#pragma once

#include <oscar/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class WarpingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit WarpingTab(std::weak_ptr<TabHost>);
        WarpingTab(WarpingTab const&) = delete;
        WarpingTab(WarpingTab&&) noexcept;
        WarpingTab& operator=(WarpingTab const&) = delete;
        WarpingTab& operator=(WarpingTab&&) noexcept;
        ~WarpingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}