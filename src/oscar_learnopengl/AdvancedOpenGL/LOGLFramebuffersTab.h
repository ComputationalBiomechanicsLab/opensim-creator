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
    class LOGLFramebuffersTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLFramebuffersTab(ParentPtr<ITabHost> const&);
        LOGLFramebuffersTab(LOGLFramebuffersTab const&) = delete;
        LOGLFramebuffersTab(LOGLFramebuffersTab&&) noexcept;
        LOGLFramebuffersTab& operator=(LOGLFramebuffersTab const&) = delete;
        LOGLFramebuffersTab& operator=(LOGLFramebuffersTab&&) noexcept;
        ~LOGLFramebuffersTab() noexcept override;

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
