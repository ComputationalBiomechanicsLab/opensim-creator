#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class TabHost; }

namespace osc
{
    class LOGLGammaTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLGammaTab(ParentPtr<TabHost> const&);
        LOGLGammaTab(LOGLGammaTab const&) = delete;
        LOGLGammaTab(LOGLGammaTab&&) noexcept;
        LOGLGammaTab& operator=(LOGLGammaTab const&) = delete;
        LOGLGammaTab& operator=(LOGLGammaTab&&) noexcept;
        ~LOGLGammaTab() noexcept override;

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
