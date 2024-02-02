#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLPBRLightingTexturedTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLPBRLightingTexturedTab(ParentPtr<ITabHost> const&);
        LOGLPBRLightingTexturedTab(LOGLPBRLightingTexturedTab const&) = delete;
        LOGLPBRLightingTexturedTab(LOGLPBRLightingTexturedTab&&) noexcept;
        LOGLPBRLightingTexturedTab& operator=(LOGLPBRLightingTexturedTab const&) = delete;
        LOGLPBRLightingTexturedTab& operator=(LOGLPBRLightingTexturedTab&&) noexcept;
        ~LOGLPBRLightingTexturedTab() noexcept override;

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
