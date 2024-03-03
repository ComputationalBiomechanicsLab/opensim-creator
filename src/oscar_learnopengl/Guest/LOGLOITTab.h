#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <SDL_events.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLOITTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLOITTab(ParentPtr<ITabHost> const&);
        LOGLOITTab(LOGLOITTab const&) = delete;
        LOGLOITTab(LOGLOITTab&&) noexcept;
        LOGLOITTab& operator=(LOGLOITTab const&) = delete;
        LOGLOITTab& operator=(LOGLOITTab&&) noexcept;
        ~LOGLOITTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
