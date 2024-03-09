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
    class FrustrumCullingTab final : public ITab {
    public:
        static CStringView id();

        explicit FrustrumCullingTab(ParentPtr<ITabHost> const&);
        FrustrumCullingTab(FrustrumCullingTab const&) = delete;
        FrustrumCullingTab(FrustrumCullingTab&&) noexcept;
        FrustrumCullingTab& operator=(FrustrumCullingTab const&) = delete;
        FrustrumCullingTab& operator=(FrustrumCullingTab&&) noexcept;
        ~FrustrumCullingTab() noexcept override;

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
