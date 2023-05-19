#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class ImGuiDemoTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit ImGuiDemoTab(std::weak_ptr<TabHost>);
        ImGuiDemoTab(ImGuiDemoTab const&) = delete;
        ImGuiDemoTab(ImGuiDemoTab&&) noexcept;
        ImGuiDemoTab& operator=(ImGuiDemoTab const&) = delete;
        ImGuiDemoTab& operator=(ImGuiDemoTab&&) noexcept;
        ~ImGuiDemoTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}