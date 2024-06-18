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
    class PreviewExperimentalDataTab final : public ITab {
    public:
        static CStringView id() noexcept;

        explicit PreviewExperimentalDataTab(const ParentPtr<ITabHost>&);
        PreviewExperimentalDataTab(const PreviewExperimentalDataTab&) = delete;
        PreviewExperimentalDataTab(PreviewExperimentalDataTab&&) noexcept;
        PreviewExperimentalDataTab& operator=(const PreviewExperimentalDataTab&) = delete;
        PreviewExperimentalDataTab& operator=(PreviewExperimentalDataTab&&) noexcept;
        ~PreviewExperimentalDataTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(const SDL_Event&) final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
