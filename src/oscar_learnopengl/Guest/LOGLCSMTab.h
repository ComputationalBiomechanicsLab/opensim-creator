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
    class LOGLCSMTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLCSMTab(ParentPtr<ITabHost> const&);
        LOGLCSMTab(LOGLCSMTab const&) = delete;
        LOGLCSMTab(LOGLCSMTab&&) noexcept;
        LOGLCSMTab& operator=(LOGLCSMTab const&) = delete;
        LOGLCSMTab& operator=(LOGLCSMTab&&) noexcept;
        ~LOGLCSMTab() noexcept override;

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
