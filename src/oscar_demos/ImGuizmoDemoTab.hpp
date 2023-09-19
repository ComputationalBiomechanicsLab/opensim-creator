#pragma once

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class TabHost; }

namespace osc
{
    class ImGuizmoDemoTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit ImGuizmoDemoTab(ParentPtr<TabHost> const&);
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
