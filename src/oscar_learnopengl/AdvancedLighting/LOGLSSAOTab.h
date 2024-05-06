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
    class LOGLSSAOTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLSSAOTab(ParentPtr<ITabHost> const&);
        LOGLSSAOTab(LOGLSSAOTab const&) = delete;
        LOGLSSAOTab(LOGLSSAOTab&&) noexcept;
        LOGLSSAOTab& operator=(LOGLSSAOTab const&) = delete;
        LOGLSSAOTab& operator=(LOGLSSAOTab&&) noexcept;
        ~LOGLSSAOTab() noexcept override;

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
