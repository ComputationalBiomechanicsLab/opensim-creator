#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererSDFTab final : public Tab {
    public:
        static CStringView id() noexcept;

        RendererSDFTab(TabHost*);
        RendererSDFTab(RendererSDFTab const&) = delete;
        RendererSDFTab(RendererSDFTab&&) noexcept;
        RendererSDFTab& operator=(RendererSDFTab const&) = delete;
        RendererSDFTab& operator=(RendererSDFTab&&) noexcept;
        ~RendererSDFTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}