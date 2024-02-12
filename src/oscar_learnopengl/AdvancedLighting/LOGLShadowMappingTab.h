#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLShadowMappingTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLShadowMappingTab(ParentPtr<ITabHost> const&);
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
