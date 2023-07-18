#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererSDFTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererSDFTab(std::weak_ptr<TabHost>);
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
