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

        explicit MandelbrotTab(const ParentPtr<ITabHost>&);
        MandelbrotTab(const MandelbrotTab&) = delete;
        MandelbrotTab(MandelbrotTab&&) noexcept;
        MandelbrotTab& operator=(const MandelbrotTab&) = delete;
        MandelbrotTab& operator=(MandelbrotTab&&) noexcept;
        ~MandelbrotTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        bool impl_on_event(const SDL_Event&) final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
