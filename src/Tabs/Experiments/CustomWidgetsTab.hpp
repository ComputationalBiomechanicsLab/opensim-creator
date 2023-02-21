#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class CustomWidgetsTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit CustomWidgetsTab(std::weak_ptr<TabHost>);
        CustomWidgetsTab(CustomWidgetsTab const&) = delete;
        CustomWidgetsTab(CustomWidgetsTab&&) noexcept;
        CustomWidgetsTab& operator=(CustomWidgetsTab const&) = delete;
        CustomWidgetsTab& operator=(CustomWidgetsTab&&) noexcept;
        ~CustomWidgetsTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}