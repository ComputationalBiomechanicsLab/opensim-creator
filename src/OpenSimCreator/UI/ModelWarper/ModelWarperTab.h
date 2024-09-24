#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc::mow
{
    class ModelWarperTab final : public ITab {
    public:
        static CStringView id();

        explicit ModelWarperTab(const ParentPtr<ITabHost>&);
        ModelWarperTab(const ModelWarperTab&) = delete;
        ModelWarperTab(ModelWarperTab&&) noexcept;
        ModelWarperTab& operator=(const ModelWarperTab&) = delete;
        ModelWarperTab& operator=(ModelWarperTab&&) noexcept;
        ~ModelWarperTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
