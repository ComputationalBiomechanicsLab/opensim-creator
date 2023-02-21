#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererTexturingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererTexturingTab(std::weak_ptr<TabHost>);
        RendererTexturingTab(RendererTexturingTab const&) = delete;
        RendererTexturingTab(RendererTexturingTab&&) noexcept;
        RendererTexturingTab& operator=(RendererTexturingTab const&) = delete;
        RendererTexturingTab& operator=(RendererTexturingTab&&) noexcept;
        ~RendererTexturingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
