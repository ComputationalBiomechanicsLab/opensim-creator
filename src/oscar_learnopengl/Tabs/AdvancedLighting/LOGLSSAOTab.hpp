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
    class LOGLSSAOTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLSSAOTab(ParentPtr<TabHost> const&);
        LOGLSSAOTab(LOGLSSAOTab const&) = delete;
        LOGLSSAOTab(LOGLSSAOTab&&) noexcept;
        LOGLSSAOTab& operator=(LOGLSSAOTab const&) = delete;
        LOGLSSAOTab& operator=(LOGLSSAOTab&&) noexcept;
        ~LOGLSSAOTab() noexcept override;

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
