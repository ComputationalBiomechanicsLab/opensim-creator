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
    class LOGLParallaxMappingTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLParallaxMappingTab(ParentPtr<ITabHost> const&);
        LOGLParallaxMappingTab(LOGLParallaxMappingTab const&) = delete;
        LOGLParallaxMappingTab(LOGLParallaxMappingTab&&) noexcept;
        LOGLParallaxMappingTab& operator=(LOGLParallaxMappingTab const&) = delete;
        LOGLParallaxMappingTab& operator=(LOGLParallaxMappingTab&&) noexcept;
        ~LOGLParallaxMappingTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(SDL_Event const&) final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
