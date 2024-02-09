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
    class LOGLPBRSpecularIrradianceTexturedTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLPBRSpecularIrradianceTexturedTab(ParentPtr<ITabHost> const&);
        LOGLPBRSpecularIrradianceTexturedTab(LOGLPBRSpecularIrradianceTexturedTab const&) = delete;
        LOGLPBRSpecularIrradianceTexturedTab(LOGLPBRSpecularIrradianceTexturedTab&&) noexcept;
        LOGLPBRSpecularIrradianceTexturedTab& operator=(LOGLPBRSpecularIrradianceTexturedTab const&) = delete;
        LOGLPBRSpecularIrradianceTexturedTab& operator=(LOGLPBRSpecularIrradianceTexturedTab&&) noexcept;
        ~LOGLPBRSpecularIrradianceTexturedTab() noexcept override;

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
