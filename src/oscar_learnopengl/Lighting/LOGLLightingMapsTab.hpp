#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLLightingMapsTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLLightingMapsTab(ParentPtr<ITabHost> const&);
        LOGLLightingMapsTab(LOGLLightingMapsTab const&) = delete;
        LOGLLightingMapsTab(LOGLLightingMapsTab&&) noexcept;
        LOGLLightingMapsTab& operator=(LOGLLightingMapsTab const&) = delete;
        LOGLLightingMapsTab& operator=(LOGLLightingMapsTab&&) noexcept;
        ~LOGLLightingMapsTab() noexcept override;

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
