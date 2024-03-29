#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class ImGuiDemoTab final : public ITab {
    public:
        static CStringView id();

        explicit ImGuiDemoTab(ParentPtr<ITabHost> const&);
        ImGuiDemoTab(ImGuiDemoTab const&) = delete;
        ImGuiDemoTab(ImGuiDemoTab&&) noexcept;
        ImGuiDemoTab& operator=(ImGuiDemoTab const&) = delete;
        ImGuiDemoTab& operator=(ImGuiDemoTab&&) noexcept;
        ~ImGuiDemoTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
