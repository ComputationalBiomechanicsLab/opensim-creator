#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class LOGLHDRTab final : public ITab {
    public:
        static CStringView id();

        explicit LOGLHDRTab(ParentPtr<ITabHost> const&);
        LOGLHDRTab(LOGLHDRTab const&) = delete;
        LOGLHDRTab(LOGLHDRTab&&) noexcept;
        LOGLHDRTab& operator=(LOGLHDRTab const&) = delete;
        LOGLHDRTab& operator=(LOGLHDRTab&&) noexcept;
        ~LOGLHDRTab() noexcept override;

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
