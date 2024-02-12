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
    class LOGLBloomTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLBloomTab(ParentPtr<ITabHost> const&);
        LOGLBloomTab(LOGLBloomTab const&) = delete;
        LOGLBloomTab(LOGLBloomTab&&) noexcept;
        LOGLBloomTab& operator=(LOGLBloomTab const&) = delete;
        LOGLBloomTab& operator=(LOGLBloomTab&&) noexcept;
        ~LOGLBloomTab() noexcept override;

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
