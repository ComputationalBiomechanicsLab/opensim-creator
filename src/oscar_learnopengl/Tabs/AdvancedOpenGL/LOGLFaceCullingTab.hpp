#pragma once

#include <oscar/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class TabHost; }

namespace osc
{
    class LOGLFaceCullingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLFaceCullingTab(ParentPtr<TabHost> const&);
        LOGLFaceCullingTab(LOGLFaceCullingTab const&) = delete;
        LOGLFaceCullingTab(LOGLFaceCullingTab&&) noexcept;
        LOGLFaceCullingTab& operator=(LOGLFaceCullingTab const&) = delete;
        LOGLFaceCullingTab& operator=(LOGLFaceCullingTab&&) noexcept;
        ~LOGLFaceCullingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
