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
    class MandelbrotTab final : public ITab {
    public:
        static CStringView id();

        explicit MandelbrotTab(ParentPtr<ITabHost> const&);
        MandelbrotTab(MandelbrotTab const&) = delete;
        MandelbrotTab(MandelbrotTab&&) noexcept;
        MandelbrotTab& operator=(MandelbrotTab const&) = delete;
        MandelbrotTab& operator=(MandelbrotTab&&) noexcept;
        ~MandelbrotTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        bool impl_on_event(SDL_Event const&) final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
