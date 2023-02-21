#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererHelloTriangleTab final : public Tab {
    public:
        static CStringView id() noexcept;

        RendererHelloTriangleTab(TabHost*);
        RendererHelloTriangleTab(RendererHelloTriangleTab const&) = delete;
        RendererHelloTriangleTab(RendererHelloTriangleTab&&) noexcept;
        RendererHelloTriangleTab& operator=(RendererHelloTriangleTab const&) = delete;
        RendererHelloTriangleTab& operator=(RendererHelloTriangleTab&&) noexcept;
        ~RendererHelloTriangleTab() noexcept final;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
