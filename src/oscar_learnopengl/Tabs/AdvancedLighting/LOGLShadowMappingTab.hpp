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
    class LOGLShadowMappingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLShadowMappingTab(ParentPtr<TabHost> const&);
        LOGLShadowMappingTab(LOGLShadowMappingTab const&) = delete;
        LOGLShadowMappingTab(LOGLShadowMappingTab&&) noexcept;
        LOGLShadowMappingTab& operator=(LOGLShadowMappingTab const&) = delete;
        LOGLShadowMappingTab& operator=(LOGLShadowMappingTab&&) noexcept;
        ~LOGLShadowMappingTab() noexcept override;

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
