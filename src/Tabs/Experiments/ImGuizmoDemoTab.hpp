#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class ImGuizmoDemoTab final : public Tab {
    public:
        static CStringView id() noexcept;

        ImGuizmoDemoTab(TabHost*);
        ImGuizmoDemoTab(ImGuizmoDemoTab const&) = delete;
        ImGuizmoDemoTab(ImGuizmoDemoTab&&) noexcept;
        ImGuizmoDemoTab& operator=(ImGuizmoDemoTab const&) = delete;
        ImGuizmoDemoTab& operator=(ImGuizmoDemoTab&&) noexcept;
        ~ImGuizmoDemoTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}