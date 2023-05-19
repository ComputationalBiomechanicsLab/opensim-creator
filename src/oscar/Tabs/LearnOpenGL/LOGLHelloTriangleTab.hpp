#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class LOGLHelloTriangleTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLHelloTriangleTab(std::weak_ptr<TabHost>);
        LOGLHelloTriangleTab(LOGLHelloTriangleTab const&) = delete;
        LOGLHelloTriangleTab(LOGLHelloTriangleTab&&) noexcept;
        LOGLHelloTriangleTab& operator=(LOGLHelloTriangleTab const&) = delete;
        LOGLHelloTriangleTab& operator=(LOGLHelloTriangleTab&&) noexcept;
        ~LOGLHelloTriangleTab() noexcept final;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
