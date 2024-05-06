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
    class LOGLBlendingTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLBlendingTab(ParentPtr<ITabHost> const&);
        LOGLBlendingTab(LOGLBlendingTab const&) = delete;
        LOGLBlendingTab(LOGLBlendingTab&&) noexcept;
        LOGLBlendingTab& operator=(LOGLBlendingTab const&) = delete;
        LOGLBlendingTab& operator=(LOGLBlendingTab&&) noexcept;
        ~LOGLBlendingTab() noexcept override;

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
