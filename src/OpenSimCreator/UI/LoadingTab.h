#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <filesystem>
#include <memory>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }

namespace osc
{
    class LoadingTab final : public ITab {
    public:
        LoadingTab(ParentPtr<IMainUIStateAPI> const&, std::filesystem::path);
        LoadingTab(LoadingTab const&) = delete;
        LoadingTab(LoadingTab&&) noexcept;
        LoadingTab& operator=(LoadingTab const&) = delete;
        LoadingTab& operator=(LoadingTab&&) noexcept;
        ~LoadingTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
