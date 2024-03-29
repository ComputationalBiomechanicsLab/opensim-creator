#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class ImGuizmoDemoTab final : public ITab {
    public:
        static CStringView id();

        explicit ImGuizmoDemoTab(ParentPtr<ITabHost> const&);
        ImGuizmoDemoTab(ImGuizmoDemoTab const&) = delete;
        ImGuizmoDemoTab(ImGuizmoDemoTab&&) noexcept;
        ImGuizmoDemoTab& operator=(ImGuizmoDemoTab const&) = delete;
        ImGuizmoDemoTab& operator=(ImGuizmoDemoTab&&) noexcept;
        ~ImGuizmoDemoTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
