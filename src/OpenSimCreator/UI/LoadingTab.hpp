#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <filesystem>
#include <memory>

namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }

namespace osc
{
    class LoadingTab final : public ITab {
    public:
        LoadingTab(ParentPtr<MainUIStateAPI> const&, std::filesystem::path);
        LoadingTab(LoadingTab const&) = delete;
        LoadingTab(LoadingTab&&) noexcept;
        LoadingTab& operator=(LoadingTab const&) = delete;
        LoadingTab& operator=(LoadingTab&&) noexcept;
        ~LoadingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnTick() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
