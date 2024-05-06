#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLHelloTriangleTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLHelloTriangleTab(ParentPtr<ITabHost> const&);
        LOGLHelloTriangleTab(LOGLHelloTriangleTab const&) = delete;
        LOGLHelloTriangleTab(LOGLHelloTriangleTab&&) noexcept;
        LOGLHelloTriangleTab& operator=(LOGLHelloTriangleTab const&) = delete;
        LOGLHelloTriangleTab& operator=(LOGLHelloTriangleTab&&) noexcept;
        ~LOGLHelloTriangleTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
