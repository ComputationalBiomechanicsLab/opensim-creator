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
    class MeshWarpingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit MeshWarpingTab(ParentPtr<TabHost> const&);
        MeshWarpingTab(MeshWarpingTab const&) = delete;
        MeshWarpingTab(MeshWarpingTab&&) noexcept;
        MeshWarpingTab& operator=(MeshWarpingTab const&) = delete;
        MeshWarpingTab& operator=(MeshWarpingTab&&) noexcept;
        ~MeshWarpingTab() noexcept override;

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
