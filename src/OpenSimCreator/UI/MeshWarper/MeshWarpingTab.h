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
    class MeshWarpingTab final : public ITab {
    public:
        static CStringView id();

        explicit MeshWarpingTab(ParentPtr<ITabHost> const&);
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
