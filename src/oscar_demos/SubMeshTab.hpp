#pragma once

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <SDL_events.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class TabHost; }

namespace osc
{
    class SubMeshTab final : public Tab {
    public:
        static CStringView id();

        explicit SubMeshTab(ParentPtr<TabHost> const&);
        SubMeshTab(SubMeshTab const&) = delete;
        SubMeshTab(SubMeshTab&&) noexcept;
        SubMeshTab& operator=(SubMeshTab const&) = delete;
        SubMeshTab& operator=(SubMeshTab&&) noexcept;
        ~SubMeshTab() noexcept override;

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
